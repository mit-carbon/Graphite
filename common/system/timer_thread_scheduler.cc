#include <sys/syscall.h>
#include <time.h>
#include "timer_thread_scheduler.h"
#include "thread_manager.h"
#include "tile_manager.h"
#include "simulator.h"
#include "network.h"

TimerThreadScheduler::TimerThreadScheduler(ThreadManager* thread_manager, TileManager* tile_manager) 
: ThreadScheduler(thread_manager, tile_manager)
{
   m_last_start_time.resize(m_total_tiles);
   for (UInt32 i = 0; i < m_total_tiles; i++)
      m_last_start_time[i].resize(m_threads_per_core);

   try
   {
      m_thread_switch_time = (UInt64) Sim()->getCfg()->getInt("thread_scheduling/quantum"); 
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error Reading 'thread_scheduling/quantum' from the config file");
   }
}

TimerThreadScheduler::~TimerThreadScheduler()
{
}

void TimerThreadScheduler::yieldThread()
{
   LOG_PRINT("In TimerThreadScheduler::yieldThread()");
   Core* core = m_tile_manager->getCurrentCore();
   core_id_t core_id = m_tile_manager->getCurrentCoreID();
   thread_id_t thread_idx = m_tile_manager->getCurrentThreadIndex();

   // Core 0 is not allowed to be multithreaded or yield.
   if (core_id.tile_id == 0 && m_tile_manager->isMainCore(core_id))
      return;

   ThreadYieldRequest req = { MCP_MESSAGE_THREAD_YIELD_REQUEST,
                              core_id,
                              thread_idx,
                              INVALID_THREAD_ID,
                              INVALID_CORE_ID,
                              INVALID_THREAD_ID,
                              INVALID_THREAD_ID
                           };

   Network *net = core->getNetwork();
   core_id_t mcp_core = Config::getSingleton()->getMCPCoreId();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   LOG_PRINT("elau: %i on {%i, %i} about to wait for message from masterYield", thread_idx, core_id.tile_id, core_id.core_type);
   NetPacket pkt = net->netRecvType(MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE);
   LOG_PRINT("elau: %i on {%i, %i} just received message from masterYield", thread_idx, core_id.tile_id, core_id.core_type);

   LOG_PRINT("acquiring m_core_lock[%i]", core_id.tile_id);
   m_core_lock[core_id.tile_id].acquire();
   LOG_PRINT("acquired m_core_lock[%i]", core_id.tile_id);
   //LOG_ASSERT_ERROR(pkt.length == sizeof(core_id_t) + sizeof(thread_id_t) + sizeof(m_local_next_tidx), "Unexpected reply size. Got %i but expecting %i", pkt.length,  sizeof(core_id_t) + sizeof(thread_id_t) + sizeof(m_local_next_tidx));

   ThreadYieldRequest * reply = (ThreadYieldRequest*) ((Byte*)pkt.data);
   core_id_t req_core_id      = reply->requester;
   thread_id_t req_thread_idx = reply->requester_tidx;
   thread_id_t req_next_tidx  = reply->requester_next_tidx;

   assert(req_core_id.tile_id == core_id.tile_id && req_core_id.core_type == core_id.core_type && req_thread_idx == thread_idx);

   core_id_t dst_core_id      = reply->destination;
   thread_id_t dst_thread_idx = reply->destination_tidx;
   thread_id_t dst_next_tidx  = reply->destination_next_tidx;

   // Set next tidx on current running process.
   m_local_next_tidx[core_id.tile_id] = req_next_tidx;
   m_local_next_tidx[dst_core_id.tile_id] = dst_next_tidx;
   
   // If this thread has migrated, update local data structures.
   if(dst_core_id.tile_id != core_id.tile_id)
   {
      m_core_lock[core_id.tile_id].release();
      m_core_lock[dst_core_id.tile_id].acquire();
      m_tile_manager->updateTLS(dst_core_id.tile_id, dst_thread_idx, m_tile_manager->getCurrentThreadId());
   }

   m_thread_wait_cond[req_core_id.tile_id].broadcast();

   while (dst_thread_idx != m_local_next_tidx[dst_core_id.tile_id])
   {
      m_tile_manager->getCurrentCore()->setState(Core::STALLED);
      LOG_PRINT("Thread id %i going to sleep as %i core {%i, %i}, with idx %i in front of the queue", m_tile_manager->getCurrentThreadId(), dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type,  m_local_next_tidx[dst_core_id.tile_id] );
      m_thread_wait_cond[dst_core_id.tile_id].wait(m_core_lock[dst_core_id.tile_id]);
      LOG_PRINT("Thread id %i waking up, went to sleep as %i core {%i, %i}", m_tile_manager->getCurrentThreadId(), dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type);
   }

   m_tile_manager->getCurrentCore()->setState(Core::RUNNING);

   LOG_PRINT("Resuming thread %i on {%i, %i}", dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type);
   m_core_lock[dst_core_id.tile_id].release();
}

void TimerThreadScheduler::masterYieldThread(ThreadYieldRequest* req)
{
   core_id_t req_core_id = req->requester;
   thread_id_t requester_tidx = req->requester_tidx;
   m_local_next_tidx[req_core_id.tile_id] = requester_tidx;
   core_id_t dst_core_id = req->requester;
   thread_id_t destination_tidx = req->requester_tidx;

   LOG_PRINT("In TimerThreadScheduler::masterYieldThread() for %i on {%i, %i}", requester_tidx, req_core_id.tile_id, req_core_id.core_type);

   LOG_ASSERT_ERROR(m_master, "TimerThreadScheduler::masterYieldThread should only be called on master.");

   std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

   m_core_lock[req_core_id.tile_id].acquire();

   UInt32 current_time = (UInt32) time(NULL);
   if (current_time - m_last_start_time[req_core_id.tile_id][requester_tidx] >= m_thread_switch_time)
   {
      assert(m_waiter_queue[req_core_id.tile_id].front()->destination_tidx == requester_tidx);
      LOG_PRINT("Yielding thread %i on {%i, %i}", requester_tidx, req_core_id.tile_id, req_core_id.core_type); 
      m_last_start_time[req_core_id.tile_id][requester_tidx] = current_time;

      m_thread_manager->stallThread(req_core_id, requester_tidx);


      // Requeue this thread request.
      ThreadSpawnRequest *req_cpy;
      req_cpy = m_waiter_queue[req_core_id.tile_id].front();
      m_waiter_queue[req_core_id.tile_id].pop();
      m_waiter_queue[req_core_id.tile_id].push(req_cpy);

      // Check if the next thread is new, or simply stalled from an earlier yield.
      std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

      bool is_thread_new = false;
      m_local_next_tidx[req_core_id.tile_id] = m_waiter_queue[req_core_id.tile_id].front()->destination_tidx;

      // Migrate this thread if it's affinity is set to another core.
      this->masterCheckAffinityAndMigrate(req_core_id, requester_tidx, dst_core_id, destination_tidx);

      // If the queue is empty, that means all threads have moved, there is no next_tidx
      if (m_waiter_queue[req_core_id.tile_id].empty())
      {
         m_local_next_tidx[req_core_id.tile_id] = INVALID_THREAD_ID;
      }
      else
      {
         if (m_tile_manager->isMainCore(req_core_id))
            is_thread_new = thread_state[req_core_id.tile_id][m_local_next_tidx[req_core_id.tile_id]].status == Core::INITIALIZING;

         if (is_thread_new)
            masterStartThread(req_core_id);
         else
            m_thread_manager->resumeThread(req_core_id, m_local_next_tidx[req_core_id.tile_id]);
      }
   }
   ThreadYieldRequest reply = {  MCP_MESSAGE_THREAD_YIELD_REQUEST,
                                 req_core_id,
                                 requester_tidx,
                                 m_local_next_tidx[req_core_id.tile_id],
                                 dst_core_id,
                                 destination_tidx,
                                 m_local_next_tidx[dst_core_id.tile_id]
                              };

   m_tile_manager->getCurrentCore()->getNetwork()->netSend( req_core_id, 
                                                            MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE,
                                                            &reply,
                                                            sizeof(reply));

   m_core_lock[req_core_id.tile_id].release();
}

