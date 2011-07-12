#include <sys/syscall.h>
#include <time.h>
#include "thread_scheduler.h"
#include "thread_manager.h"
#include "timer_thread_scheduler.h"
#include "tile_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "simulator.h"
#include "mcp.h"
#include "network.h"
#include "message_types.h"
#include "tile.h"
#include "core.h"
#include "thread.h"

ThreadScheduler* ThreadScheduler::create(ThreadManager *thread_manager, TileManager *tile_manager)
{
   std::string scheme = Sim()->getCfg()->getString("thread_scheduling/scheme");
   ThreadScheduler* thread_scheduler = NULL;

   if (scheme == "timer") {
      thread_scheduler = new TimerThreadScheduler(thread_manager, tile_manager);
   }
   else if (scheme == "none") {
      thread_scheduler = new ThreadScheduler(thread_manager, tile_manager);
   }
   else {
      LOG_PRINT_ERROR("Unrecognized thread scheduling scheme: %s", scheme.c_str());
   }
   return thread_scheduler;
}

ThreadScheduler::ThreadScheduler(ThreadManager *thread_manager, TileManager *tile_manager)
{
   Config *config = Config::getSingleton();

   m_master = config->getCurrentProcessNum() == 0;

   m_thread_manager = thread_manager;
   m_thread_manager->setThreadScheduler(this);
   m_tile_manager = tile_manager;

   m_total_tiles = config->getTotalTiles();
   m_threads_per_core = config->getMaxThreadsPerCore();

   m_core_lock.resize(m_total_tiles);
   m_thread_wait_cond.resize(m_total_tiles);

   m_local_next_tidx.resize(m_total_tiles);
   for (UInt32 i = 0; i < m_total_tiles; i++)
      m_local_next_tidx[i] = INVALID_THREAD_ID;

   m_last_start_time.resize(m_total_tiles);
   for (UInt32 i = 0; i < m_total_tiles; i++)
      m_last_start_time[i].resize(m_threads_per_core);

   for (UInt32 i = 0; i < m_total_tiles; i++)
      for (UInt32 j = 0; j < m_threads_per_core; j++)
         m_last_start_time[i][j] = 0;

   m_waiter_queue.resize(m_total_tiles);

   try
   {
      m_thread_switch_time = (UInt64) Sim()->getCfg()->getInt("thread_scheduling/quantum"); 
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Error Reading 'thread_scheduling/quantum' from the config file");
   }
}

ThreadScheduler::~ThreadScheduler()
{
}

void ThreadScheduler::onThreadExit()
{
   if (m_tile_manager->getCurrentTileID() != Sim()->getConfig()->getCurrentThreadSpawnerTileNum() && m_tile_manager->getCurrentTileID() != Sim()->getConfig()->getMCPTileNum())
   {
      // Wait for master to update the next thread
      Network *net = m_tile_manager->getCurrentCore()->getNetwork();
      NetPacket pkt = net->netRecvType(MCP_THREAD_EXIT_REPLY_FROM_MASTER_TYPE);
      LOG_ASSERT_ERROR(pkt.length == sizeof(core_id_t) + sizeof(thread_id_t), "Unexpected reply size.");

      core_id_t dst_core_id = *(core_id_t*)((Byte*)pkt.data);
      thread_id_t dst_thread_idx = *(thread_id_t*)((Byte*)pkt.data+sizeof(core_id_t));
      assert(dst_core_id.tile_id == m_tile_manager->getCurrentCoreID().tile_id && dst_core_id.core_type == m_tile_manager->getCurrentCoreID().core_type);

      // Set next tidx on current running process.
      m_local_next_tidx[dst_core_id.tile_id] = dst_thread_idx;

      if (dst_thread_idx != INVALID_THREAD_ID)
         m_thread_wait_cond[dst_core_id.tile_id].broadcast();
   }
}

void ThreadScheduler::masterOnThreadExit(core_id_t core_id, SInt32 thread_idx)
{
   LOG_ASSERT_ERROR(m_master, "onThreadExit should only be called on master.");
   LOG_PRINT("In ThreadScheduler::masterOnThreadExit thread %i on {%i, %i}", thread_idx, core_id.tile_id, core_id.core_type); 
   m_core_lock[core_id.tile_id].acquire();

   assert(m_thread_manager->isCoreRunning(core_id) == INVALID_THREAD_ID);
   if (core_id.tile_id != Sim()->getConfig()->getCurrentThreadSpawnerTileNum() && core_id.tile_id != Sim()->getConfig()->getMCPTileNum())
      if (!m_waiter_queue[core_id.tile_id].empty())
         m_waiter_queue[core_id.tile_id].pop(); 

   thread_id_t next_tidx = INVALID_THREAD_ID; 
   if (!m_waiter_queue[core_id.tile_id].empty())
   {
      next_tidx = m_waiter_queue[core_id.tile_id].front()->destination_tidx;

      if (next_tidx != INVALID_THREAD_ID)
      {
         std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();
         bool is_thread_new = false;

         // Check if the next thread is new, or simply stalled from an earlier yield.
         if (m_tile_manager->isMainCore(core_id))
            is_thread_new = thread_state[core_id.tile_id][next_tidx].status == Core::INITIALIZING;

         if (is_thread_new)
            masterStartThread(core_id);
         else
            m_thread_manager->resumeThread(core_id, next_tidx);
      }
   }

   SInt32 msg[] = {core_id.tile_id, core_id.core_type, next_tidx};

   Core *core = m_tile_manager->getCurrentCore();
   core->getNetwork()->netSend(core_id, 
         MCP_THREAD_EXIT_REPLY_FROM_MASTER_TYPE,
         msg,
         sizeof(core_id.tile_id)+sizeof(core_id.core_type)+sizeof(next_tidx));

   m_core_lock[core_id.tile_id].release();
}

void ThreadScheduler::masterScheduleThread(ThreadSpawnRequest *req)
{
   LOG_ASSERT_ERROR(m_master, "masterScheduleThread should only be called on master.");
   LOG_PRINT("(3) masterScheduleThread for (%i) on core id (%i, %i)", req->destination_tidx, req->destination.tile_id, req->destination.core_type);

   // Get lock on destination tile.
   m_core_lock[req->destination.tile_id].acquire();

   bool is_thread_startable = false;
   std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

   // Add thread to spawning queue.
   if (req->destination.tile_id != Sim()->getConfig()->getCurrentThreadSpawnerTileNum())
   {
      ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
      *req_cpy = *req;
      m_waiter_queue[req->destination.tile_id].push(req_cpy);
   }

   // Now determine if we can actually start running this thread!
   thread_id_t running_thread = INVALID_THREAD_ID;
   if (req->requester.tile_id == req->destination.tile_id)
   {
      // If the requesting tile is the destination tile, the calling thread is stalled, but should be set as the
      // running thread because it will resume after the spawn call.
      if (m_tile_manager->isMainCore(req->requester))
         running_thread = thread_state[req->requester.tile_id][req->requester_tidx].thread_id;
   }
   else
   {
      // Otherwise check if the destination core has a thread already running or about to run.
      running_thread = m_thread_manager->isCoreRunning(req->destination);
      if (running_thread == INVALID_THREAD_ID)
         is_thread_startable = !m_thread_manager->isCoreInitializing(req->destination);
   }

   // Grab the thread states on destination tile and set to initializing.

   if (m_tile_manager->isMainCore(req->destination))
   {
      if (thread_state[req->destination.tile_id][req->destination_tidx].status == Core::IDLE) 
         m_thread_manager->setThreadState(req->destination.tile_id, req->destination_tidx, Core::INITIALIZING);
   }
   else
      LOG_ASSERT_ERROR(false, "Invalid core type");

   // Start this thread if the core is idle.
   if (is_thread_startable)
      this->masterStartThread(req->destination);

   m_core_lock[req->destination.tile_id].release();
}

void ThreadScheduler::masterStartThread(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "masterStartThread should only be called on master.");
   
   // Start the thread next in line.
   LOG_ASSERT_ERROR(!m_waiter_queue[core_id.tile_id].empty(), "m_waiter_queue[%i].size() = %d", core_id.tile_id, m_waiter_queue[core_id.tile_id].size() );
   ThreadSpawnRequest *req = m_waiter_queue[core_id.tile_id].front();
 
   // Grab the thread states on destination tile.
   std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

   m_last_start_time[req->destination.tile_id][req->destination_tidx] = (UInt32) time(NULL);

   // Spawn the thread by calling LCP on correct process.
   LOG_PRINT("New Thread(%i) to be spawned core id(%i, %i)", req->destination_tidx, req->destination.tile_id, req->destination.core_type);
   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {  
      // Spawn process on child
      SInt32 dest_proc = Config::getSingleton()->getProcessNumForTile(req->destination.tile_id);
      Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

      req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_MASTER;

      globalNode->globalSend(dest_proc, req, sizeof(*req));
   }
   else // if (Sim()->getConfig()->getSimulationMode() == Config::LITE)
   {
      ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
      *req_cpy = *req;
      
      // Insert the request in the thread request queue and the thread request map
      m_thread_manager->insertThreadSpawnRequest(req_cpy);
      m_thread_manager->m_thread_spawn_sem.signal();
         
      SInt32 msg[] = { req->destination.tile_id, req->destination.core_type, req->destination_tidx};

      Core *core = m_tile_manager->getCurrentCore();
      core->getNetwork()->netSend(req->requester, 
                                  MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE,
                                  msg,
                                  sizeof(req->destination.tile_id)+sizeof(req->destination.core_type)+sizeof(req->destination_tidx));
   }

}

void ThreadScheduler::yieldThread()
{
   LOG_PRINT_ERROR("No scheme was set for yielding threads!");
}

void ThreadScheduler::masterYieldThread(ThreadYieldRequest* req)
{
   LOG_PRINT_ERROR("No scheme was set for yielding threads!");
}
