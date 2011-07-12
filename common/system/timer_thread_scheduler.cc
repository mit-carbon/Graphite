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

   ThreadYieldRequest req = { MCP_MESSAGE_THREAD_YIELD_REQUEST_FROM_REQUESTER,
                              core_id,
                              thread_idx };

   Network *net = core->getNetwork();
   core_id_t mcp_core = Config::getSingleton()->getMCPCoreId();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   NetPacket pkt = net->netRecvType(MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE);

   m_core_lock[core_id.tile_id].acquire();
   LOG_ASSERT_ERROR(pkt.length == sizeof(core_id_t) + sizeof(thread_id_t), "Unexpected reply size.");
   core_id_t dst_core_id = *(core_id_t*)((Byte*)pkt.data);
   thread_id_t dst_thread_idx = *(thread_id_t*)((Byte*)pkt.data+sizeof(core_id_t));
   assert(dst_core_id.tile_id == core_id.tile_id && dst_core_id.core_type == core_id.core_type);

   // Set next tidx on current running process.
   m_local_next_tidx[core_id.tile_id] = dst_thread_idx;

   m_thread_wait_cond[core_id.tile_id].broadcast();
   while (thread_idx != m_local_next_tidx[core_id.tile_id])
      m_thread_wait_cond[core_id.tile_id].wait(m_core_lock[core_id.tile_id]);

   LOG_PRINT("Resuming thread %i on {%i, %i}", thread_idx, core_id.tile_id, core_id.core_type);
   m_core_lock[core_id.tile_id].release();
}

void TimerThreadScheduler::masterYieldThread(ThreadYieldRequest* req)
{
   core_id_t req_core_id = req->requester;
   thread_id_t requester_tidx = req->requester_tidx;
   thread_id_t next_tidx = requester_tidx;

   LOG_ASSERT_ERROR(m_master, "TimerThreadScheduler::masterYieldThread should only be called on master.");

   std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

   m_core_lock[req_core_id.tile_id].acquire();

   UInt32 current_time = (UInt32) time(NULL);
   if (m_waiter_queue[req_core_id.tile_id].size() > 1 && current_time - m_last_start_time[req_core_id.tile_id][requester_tidx] >= m_thread_switch_time)
   {
      assert(m_waiter_queue[req_core_id.tile_id].front()->destination_tidx == requester_tidx);
      LOG_PRINT("Yielding thread %i on {%i, %i}", requester_tidx, req_core_id.tile_id, req_core_id.core_type); 
      m_last_start_time[req_core_id.tile_id][requester_tidx] = current_time;

      m_thread_manager->stallThread(req_core_id, requester_tidx);
      m_tile_manager->getCoreFromID(req_core_id)->setState(Core::STALLED);


      // Requeue this thread request.
      ThreadSpawnRequest *req_cpy;
      req_cpy = m_waiter_queue[req_core_id.tile_id].front();
      m_waiter_queue[req_core_id.tile_id].pop();
      m_waiter_queue[req_core_id.tile_id].push(req_cpy);

      // Check if the next thread is new, or simply stalled from an earlier yield.
      std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

      bool is_thread_new = false;
      next_tidx = m_waiter_queue[req_core_id.tile_id].front()->destination_tidx;
      if (m_tile_manager->isMainCore(req_core_id))
         is_thread_new = thread_state[req_core_id.tile_id][next_tidx].status == Core::INITIALIZING;

      if (is_thread_new)
      {
         masterStartThread(req_core_id);
      }
      else
      {
         m_thread_manager->resumeThread(req_core_id, next_tidx);
         m_tile_manager->getCoreFromID(req_core_id)->setState(Core::RUNNING);
      }
   }

   SInt32 msg[] = { req_core_id.tile_id, req_core_id.core_type, next_tidx};
   m_tile_manager->getCurrentCore()->getNetwork()->netSend( req_core_id, 
                                                            MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE,
                                                            msg,
                                                            sizeof(req_core_id.tile_id)+sizeof(req_core_id.core_type)+sizeof(next_tidx));

   m_core_lock[req_core_id.tile_id].release();
}

