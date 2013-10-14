#include <cassert>
#include <sys/syscall.h>
#include <time.h>
#include "thread_scheduler.h"
#include "thread_manager.h"
#include "round_robin_thread_scheduler.h"
#include "tile_manager.h"
#include "config.h"
#include "log.h"
#include "simulator.h"
#include "mcp.h"
#include "network.h"
#include "message_types.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "thread.h"

ThreadScheduler* ThreadScheduler::create(ThreadManager *thread_manager, TileManager *tile_manager)
{
   // WARNING: Do not change this parameter. Hard-coded until multi-threading bug is fixed
   std::string scheme = "none"; // Sim()->getCfg()->getString("thread_scheduling/scheme");
   ThreadScheduler* thread_scheduler = NULL;

   if (scheme == "round_robin") {
      thread_scheduler = new RoundRobinThreadScheduler(thread_manager, tile_manager);
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

   m_thread_migration_enabled = true;
   m_thread_preemption_enabled = true;

   // WARNING: Do not change this parameter. Hard-coded until multi-threading bug is fixed
   m_thread_switch_quantum = 100; // (UInt64) Sim()->getCfg()->getInt("thread_scheduling/quantum"); 
   std::string scheme = "none"; // Sim()->getCfg()->getString("thread_scheduling/scheme");
   m_enabled = (scheme != "none");
}

ThreadScheduler::~ThreadScheduler()
{
}

void ThreadScheduler::onThreadExit()
{
   if (m_tile_manager->getCurrentTileID() != Sim()->getConfig()->getCurrentThreadSpawnerTileNum() && m_tile_manager->getCurrentTileID() != Sim()->getConfig()->getMCPTileNum())
   {
      // Wait for master to update the next thread
      Core *core = m_tile_manager->getCurrentCore();
      Network *net = core->getTile()->getNetwork();
      NetPacket pkt = net->netRecvType(MCP_THREAD_EXIT_REPLY_FROM_MASTER_TYPE, core->getId());
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
         if (Tile::isMainCore(core_id))
            is_thread_new = thread_state[core_id.tile_id][next_tidx].status == Core::INITIALIZING;

         if (is_thread_new) {
            masterStartThread(core_id);
         }
         else {
            m_thread_manager->resumeThread(core_id, next_tidx);
         }
      }
   }

   SInt32 msg[] = {core_id.tile_id, core_id.core_type, next_tidx};

   Core *core = m_tile_manager->getCurrentCore();
   core->getTile()->getNetwork()->netSend(core_id, MCP_THREAD_EXIT_REPLY_FROM_MASTER_TYPE,
                                          msg, sizeof(core_id.tile_id) + sizeof(core_id.core_type) + sizeof(next_tidx));

   m_core_lock[core_id.tile_id].release();

   LOG_PRINT("Done ThreadScheduler::masterOnThreadExit lock for thread %i on {%i, %i}", thread_idx, core_id.tile_id, core_id.core_type); 
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
      if (Tile::isMainCore(req->requester))
         running_thread = thread_state[req->requester.tile_id][req->requester_tidx].thread_id;
   }
   else
   {
      // Otherwise check if the destination core has a thread already running or about to run.
      running_thread = m_thread_manager->isCoreRunning(req->destination);
      if (running_thread == INVALID_THREAD_ID)
         is_thread_startable = (!m_thread_manager->isCoreInitializing(req->destination) && !m_thread_manager->isCoreStalled(req->destination));
   }

   // Grab the thread states on destination tile and set to initializing.
   if (Tile::isMainCore(req->destination))
   {
      LOG_ASSERT_ERROR(thread_state[req->destination.tile_id][req->destination_tidx].status == Core::IDLE,
                       "Spawning a non-idle thread at %i on {%i, %i}",
                       req->destination_tidx, req->destination.tile_id, req->destination.core_type); 
      m_thread_manager->setThreadState(req->destination.tile_id, req->destination_tidx, Core::INITIALIZING);
   }
   else
   {
      LOG_PRINT_ERROR("Invalid core type");
   }

   // Start this thread if the core is idle.
   if (is_thread_startable)
   {
      masterStartThread(req->destination);
   }

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
   LOG_ASSERT_ERROR(thread_state[req->destination.tile_id][req->destination_tidx].status == Core::INITIALIZING || thread_state[req->destination.tile_id][req->destination_tidx].status == Core::STALLED, "Haven't made this work for starting waiting threads yet, current status %i", thread_state[req->destination.tile_id][req->destination_tidx].status);

   LOG_PRINT("Thread(%i) to be started on  core id(%i, %i)", req->destination_tidx, req->destination.tile_id, req->destination.core_type);
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
   }

}

void ThreadScheduler::migrateThread(thread_id_t thread_id, tile_id_t tile_id)
{
   core_id_t core_id = Tile::getMainCoreId(tile_id);

   // Send message to master process to update the waiter queues, the local thread keeps running and DOESN'T wait.
   SInt32 msg[] = { MCP_MESSAGE_THREAD_MIGRATE_REQUEST_FROM_REQUESTER, thread_id, core_id.tile_id, core_id.core_type};

   LOG_PRINT("migrateThread -- send message to master ThreadScheduler; tid %i to migrate to {%d, %d} at time %llu",
             thread_id,
             core_id.tile_id,
             core_id.core_type,
             m_tile_manager->getCurrentCore()->getModel()->getCurrTime().toNanosec());

   Network *net = m_tile_manager->getCurrentTile()->getNetwork();

   // update global thread state
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                msg,
                sizeof(SInt32) + sizeof(thread_id_t) + sizeof(tile_id_t) + sizeof(UInt32));
}

void ThreadScheduler::masterMigrateThread(thread_id_t src_thread_id, tile_id_t dst_tile_id, UInt32 dst_core_type)
{
   thread_id_t src_thread_idx = INVALID_THREAD_ID;
   core_id_t   src_core_id    = INVALID_CORE_ID;
   core_id_t   dst_core_id    = (core_id_t) {dst_tile_id, dst_core_type}; 

   // Check if destination core has threads open, if not, ignore this request.
   thread_id_t dst_thread_idx = m_thread_manager->getIdleThread(dst_core_id);

   if (dst_thread_idx != INVALID_THREAD_ID)
   {
      // Get the core and thread index of the migrating thread.
      m_thread_manager->lookupThreadIndex(src_thread_id, src_core_id, src_thread_idx);
      assert(src_thread_idx != INVALID_THREAD_ID);

      masterMigrateThread(src_thread_idx, src_core_id, dst_thread_idx, dst_core_id);
   }
}

void ThreadScheduler::masterMigrateThread(thread_id_t src_thread_idx, core_id_t src_core_id, thread_id_t dst_thread_idx, core_id_t dst_core_id)
{
   LOG_PRINT("ThreadScheduler::masterMigrateThread called to migrate %i on {%i, %i} to %i on {%i, %i}", src_thread_idx, src_core_id.tile_id, src_core_id.core_type, dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type);
   if (dst_thread_idx != INVALID_THREAD_ID)
   {
      m_core_lock[src_core_id.tile_id].acquire();
      m_core_lock[dst_core_id.tile_id].acquire();

      if (m_thread_manager->isThreadRunning(src_core_id, src_thread_idx))
      {
         LOG_ASSERT_ERROR(false, "Trying to migrate %i on {%i, %i}, but I can't migrate threads currently running!", src_thread_idx, src_core_id.tile_id, src_core_id.core_type);
      }
      else 
      {
         // Find the thread on the source core's waiter queue.
         bool found = false;
         unsigned int waiter_queue_size = 0;
         ThreadSpawnRequest * migrating_thread_req = NULL;
         waiter_queue_size = m_waiter_queue[src_core_id.tile_id].size();
         for (unsigned int i = 0; i < waiter_queue_size; i++)
         {
            ThreadSpawnRequest * req_cpy = m_waiter_queue[src_core_id.tile_id].front();
            m_waiter_queue[src_core_id.tile_id].pop();

            if (req_cpy->destination_tidx != src_thread_idx)
            {
               m_waiter_queue[src_core_id.tile_id].push(req_cpy);
            }
            else
            {
               found = true;
               migrating_thread_req = req_cpy;
            }
         }
         LOG_ASSERT_ERROR(found, "Could not find thread idx (%i) in core {%i, %i}", src_thread_idx, src_core_id.tile_id, src_core_id.core_type);
         LOG_ASSERT_ERROR(migrating_thread_req != NULL, "Request on source waiter queue was broken on (%i) in core {%i, %i}!", src_thread_idx, src_core_id.tile_id, src_core_id.core_type);


         // Get an idle thread on the destination core, and push it onto the end of the waiter queue.
         migrating_thread_req->destination.tile_id = dst_core_id.tile_id;
         migrating_thread_req->destination.core_type = dst_core_id.core_type;
         migrating_thread_req->destination_tidx = dst_thread_idx;

         m_waiter_queue[dst_core_id.tile_id].push(migrating_thread_req);

         // Start thread if it is startable.
         thread_id_t running_thread = m_thread_manager->isCoreRunning(dst_core_id);
         bool is_thread_startable = false;

         if (running_thread == INVALID_THREAD_ID)
            is_thread_startable = !m_thread_manager->isCoreInitializing(dst_core_id);

         // Update the thread states in the thread manager.
         std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

         LOG_ASSERT_ERROR(thread_state[src_core_id.tile_id][src_thread_idx].status != Core::IDLE, "The migrating thread at %i on {%i, %i} is IDLE!", src_thread_idx, src_core_id.tile_id, src_core_id.core_type);
         LOG_ASSERT_ERROR(thread_state[dst_core_id.tile_id][dst_thread_idx].status == Core::IDLE, "The destination thread at %i on {%i, %i} is not IDLE!", src_thread_idx, src_core_id.tile_id, src_core_id.core_type);

         m_thread_manager->setThreadIndex(thread_state[src_core_id.tile_id][src_thread_idx].thread_id, dst_core_id, dst_thread_idx);

         m_thread_manager->setThreadState(dst_core_id.tile_id, dst_thread_idx, thread_state[src_core_id.tile_id][src_thread_idx]);
         m_thread_manager->setThreadState(src_core_id.tile_id, src_thread_idx, Core::IDLE);

         // Thread has migrated, so the core is currently idle until a new threads starts.
         m_tile_manager->getCoreFromID(src_core_id)->setState(Core::IDLE);

         if (is_thread_startable)
         {
            m_local_next_tidx[dst_core_id.tile_id] = dst_thread_idx;
            if(m_thread_manager->getThreadState(dst_core_id.tile_id, dst_thread_idx) == Core::INITIALIZING)
            {
               masterStartThread(dst_core_id);
            }
            else
            {
               m_thread_manager->resumeThread(dst_core_id, dst_thread_idx);
               m_tile_manager->getCoreFromID(dst_core_id)->setState(Core::RUNNING);
            }
         }
      }

      m_core_lock[dst_core_id.tile_id].release();
      m_core_lock[src_core_id.tile_id].release();
   }
}

bool ThreadScheduler::schedSetAffinity(thread_id_t tid, unsigned int cpusetsize, cpu_set_t* set)
{
   LOG_PRINT("ThreadScheduler::schedSetAffinity called to set affinity for %i and total cores %i", tid, cpusetsize);
   Core* core = m_tile_manager->getCurrentCore();

   ThreadAffinityRequest req =   {  MCP_MESSAGE_THREAD_SETAFFINITY_REQUEST,
                                    core->getId(),
                                    tid,
                                    (UInt32) cpusetsize,
                                    set
                                 };

   Network *net = core->getTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   return true;
}

void ThreadScheduler::masterSchedSetAffinity(ThreadAffinityRequest* req)
{
   LOG_PRINT("ThreadScheduler::masterSchedSetAffinity called to set affinity for %i", req->tid);
   thread_id_t tid = req->tid;
   unsigned int cpusetsize = req->cpusetsize;
   cpu_set_t *set = req->cpu_set;

   UInt32 total_tiles = Config::getSingleton()->getTotalTiles();
   LOG_ASSERT_ERROR(cpusetsize <= total_tiles, "You are using a cpu mask with %i cores, but we are only simulating %d!", cpusetsize, total_tiles);

   if (cpusetsize < total_tiles)
   {
      cpu_set_t *temp_set = set;
      set = CPU_ALLOC(total_tiles);
      CPU_ZERO_S(CPU_ALLOC_SIZE(total_tiles), set);
      CPU_OR_S(CPU_ALLOC_SIZE(total_tiles), set, temp_set, set);
      CPU_FREE(temp_set);
   }


   core_id_t core_id = INVALID_CORE_ID;
   thread_id_t thread_idx = INVALID_THREAD_ID;

   m_thread_manager->lookupThreadIndex(tid, core_id, thread_idx);
   assert(thread_idx != INVALID_THREAD_ID);

   m_thread_manager->setThreadAffinity(core_id.tile_id, thread_idx, set);
}


bool ThreadScheduler::schedGetAffinity(thread_id_t tid, unsigned int cpusetsize, cpu_set_t* set)
{
   LOG_PRINT("ThreadScheduler::schedGetAffinity called to get affinity for %i and total cores %i", tid, cpusetsize);
   Core* core = m_tile_manager->getCurrentCore();

   ThreadAffinityRequest req =   {  MCP_MESSAGE_THREAD_GETAFFINITY_REQUEST,
                                    core->getId(),
                                    tid,
                                    (UInt32) cpusetsize,
                                    set
                                 };

   Network *net = core->getTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   NetPacket pkt = net->netRecvType(MCP_THREAD_GETAFFINITY_REPLY_FROM_MASTER_TYPE, core->getId());
   LOG_ASSERT_ERROR(pkt.length == sizeof(req), "Unexpected reply size (got %i expected %i).", pkt.length, sizeof(req));

   ThreadAffinityRequest * reply = (ThreadAffinityRequest*) ((Byte*)pkt.data);

   CPU_ZERO_S(CPU_ALLOC_SIZE(cpusetsize), set);
   CPU_OR_S(CPU_ALLOC_SIZE(cpusetsize), set, reply->cpu_set, set);

   return true;
}

void ThreadScheduler::masterSchedGetAffinity(ThreadAffinityRequest* req)
{
   LOG_PRINT("ThreadScheduler::masterSchedGetAffinity called to set affinity for %i", req->tid);
   thread_id_t tid = req->tid;
   cpu_set_t* set = CPU_ALLOC(req->cpusetsize);
   core_id_t core_id = INVALID_CORE_ID;
   thread_id_t thread_idx = INVALID_THREAD_ID;

   m_thread_manager->lookupThreadIndex(tid, core_id, thread_idx);
   assert(thread_idx != INVALID_THREAD_ID);
   m_thread_manager->getThreadAffinity(core_id.tile_id, thread_idx, set);
   assert(set != NULL);

   UInt32 total_tiles = Config::getSingleton()->getTotalTiles();
   LOG_ASSERT_ERROR(req->cpusetsize <= total_tiles, "You are using a cpu mask with %i cores, but we are only simulating %d!", req->cpusetsize, total_tiles);

   cpu_set_t *temp_set = set;
   set = CPU_ALLOC(total_tiles);
   CPU_ZERO_S(CPU_ALLOC_SIZE(total_tiles), set);
   CPU_OR_S(CPU_ALLOC_SIZE(total_tiles), set, temp_set, set);
   CPU_FREE(temp_set);

   ThreadAffinityRequest reply =   {  MCP_MESSAGE_THREAD_GETAFFINITY_REQUEST,
                                    req->requester,
                                    req->tid,
                                    (UInt32) req->cpusetsize,
                                    set
                                 };

   Core *core = m_tile_manager->getCurrentCore();
   core->getTile()->getNetwork()->netSend(req->requester, MCP_THREAD_GETAFFINITY_REPLY_FROM_MASTER_TYPE,
                                          &reply, sizeof(reply));
}


// Return true if affinity matches current core, return false if affinity does not match.
// This function will deal with all migrating tasks, so callers only need to take action if this returns true.
bool ThreadScheduler::masterCheckAffinityAndMigrate(core_id_t core_id, thread_id_t thread_idx, core_id_t &dst_core_id, thread_id_t &dst_thread_idx)
{
   LOG_PRINT("ThreadScheduler::masterCheckAffinityAndMigrate for %i on {%i, %i}", thread_idx, core_id.tile_id, core_id.core_type);
   bool res = true;

   dst_core_id = core_id;
   dst_thread_idx = thread_idx;

   size_t setsize = CPU_ALLOC_SIZE(m_total_tiles);
   cpu_set_t* set = CPU_ALLOC(m_total_tiles);
   m_thread_manager->getThreadAffinity(core_id.tile_id, thread_idx, set);

   // Check if this core has any affinity.
   cpu_set_t* zero_set = CPU_ALLOC(m_total_tiles);
   CPU_ZERO_S(setsize, zero_set);

   // If the current core is out of range, and there are no empty cores in the current set, use the least scheduled core.
   if ((UInt32) dst_core_id.tile_id >= m_total_tiles)
      dst_core_id = INVALID_CORE_ID;

   core_id_t least_scheduled_core_id = INVALID_CORE_ID;
   UInt32 min_scheduled_threads = 0;

   if (CPU_EQUAL_S(setsize, zero_set, set) == 0) // Yes there is affinity set to this thread.
   {
      // The cpu_set is non-empty, so check if this core is inside this threads mask set.  If the destination core is invalid that means it's out of range of the tiles currently allocated.
      if (CPU_ISSET_S(core_id.tile_id, CPU_ALLOC_SIZE(m_total_tiles), set) == 0 || dst_core_id.tile_id == INVALID_TILE_ID) 
      {
         // Find first idle core to migrate thread; if no affinity cores are idle, pick core with most available idle threads.
         core_id_t current_core_id = INVALID_CORE_ID;
         for (unsigned int i = 1; i < m_total_tiles; i++)
         {
            current_core_id =  Tile::getMainCoreId(i);
            if (CPU_ISSET_S(i, setsize, set) != 0)
            {
               if (m_thread_manager->isCoreRunning(current_core_id) == INVALID_THREAD_ID)
               {
                  dst_core_id = current_core_id;
                  break;
               }
            }

            if(least_scheduled_core_id.tile_id == INVALID_TILE_ID || m_thread_manager->getNumScheduledThreads(current_core_id) < min_scheduled_threads)
            {
               if (m_thread_manager->getIdleThread(current_core_id) != INVALID_THREAD_ID)
               {
                  min_scheduled_threads = m_thread_manager->getNumScheduledThreads(current_core_id); 
                  least_scheduled_core_id = current_core_id;
               }
            }
         }
      }
   }
   else // If no affinity is set, try to stay on same core, unless there is a totally empty core, then migrate.
   {
      core_id_t current_core_id = INVALID_CORE_ID;
      for (unsigned int i = 1; i < m_total_tiles; i++)
      {
            current_core_id =  Tile::getMainCoreId(i);

            // If core is completely empty, use it if my current core isn't also empty!
            if (m_thread_manager->getNumScheduledThreads(current_core_id) == 0)
            {
               if(dst_core_id.tile_id == INVALID_TILE_ID || m_thread_manager->getNumScheduledThreads(dst_core_id) > 1)
               {
                  dst_core_id = current_core_id;
                  break;
               }
            }

            // If we do not have a destination core yet, use the core with fewest scheduled threads.
            else if(least_scheduled_core_id.tile_id == INVALID_TILE_ID || m_thread_manager->getNumScheduledThreads(current_core_id) < min_scheduled_threads)
            {
                  if (m_thread_manager->getIdleThread(current_core_id) != INVALID_THREAD_ID)
                  {
                     min_scheduled_threads = m_thread_manager->getNumScheduledThreads(current_core_id); 
                     least_scheduled_core_id = current_core_id;
                  }
            }

      }
   }

   // If current core out of range, and out of range, migrate to least scheduled core.
   if (dst_core_id.tile_id == INVALID_TILE_ID) dst_core_id = least_scheduled_core_id;


   if ((UInt32) dst_core_id.tile_id >= m_total_tiles || dst_core_id.tile_id == INVALID_TILE_ID)
   {
      LOG_ASSERT_ERROR(false, "Thread to be scheduled on tile %i, but only %i tiles are allocated, and %i threads allowed per core.", dst_core_id.tile_id, m_total_tiles, m_threads_per_core);
   }

   if (dst_core_id.tile_id != core_id.tile_id)
   {

      dst_thread_idx = m_thread_manager->getIdleThread(dst_core_id);
      LOG_PRINT("Thread %i on {%i, %i} is moving away to %i on {%i, %i}", thread_idx, core_id.tile_id, core_id.core_type, dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type);

      m_core_lock[core_id.tile_id].release();
      masterMigrateThread(thread_idx, core_id, dst_thread_idx, dst_core_id);
      m_core_lock[core_id.tile_id].acquire();
      res = false;
   }
   else
   {
      LOG_PRINT("Thread %i on {%i, %i} is staying put.", thread_idx, core_id.tile_id, core_id.core_type);
   }

   CPU_FREE(zero_set);
   CPU_FREE(set);
   return res;
}

thread_id_t ThreadScheduler::getNextThreadIdx(core_id_t core_id) 
{
   thread_id_t next_tidx;

   if(!m_waiter_queue[core_id.tile_id].empty())
      next_tidx = m_waiter_queue[core_id.tile_id].front()->destination_tidx;
   else
      next_tidx = INVALID_THREAD_ID;

   return next_tidx;
}

// is_pre_emptive is by default true, and will honor the timer and affinity of the threads,
// if called with is_pre_emptive false, then the thread is yielded no matter the time, and it also ignores affinity and stays on the same core.
void ThreadScheduler::yieldThread(bool is_pre_emptive)
{
   if(!m_enabled) {return;}

   Core* core = m_tile_manager->getCurrentCore();
   core_id_t core_id = m_tile_manager->getCurrentCoreID();
   thread_id_t thread_idx = m_tile_manager->getCurrentThreadIndex();


   core_id_t   dst_core_id    = core_id;
   thread_id_t dst_thread_idx = thread_idx;


   // Core 0 is not allowed to be multithreaded or yield.
   if (core_id.tile_id == 0 && Tile::isMainCore(core_id))
      return;

   UInt32 current_time = (UInt32) time(NULL);
   LOG_PRINT("In ThreadScheduler::yieldThread() for thread %i on %i, time active(%i) enabled_preempt(%i)", thread_idx, core_id.tile_id, current_time - m_last_start_time[core_id.tile_id][thread_idx], m_thread_preemption_enabled);
   bool ignore_timer = !is_pre_emptive;
   if (current_time - m_last_start_time[core_id.tile_id][thread_idx] >= m_thread_switch_quantum || ignore_timer)
   {
      if(!ignore_timer) {
         if (m_thread_preemption_enabled == false) {return;}
         LOG_PRINT("ThreadScheduler::yieldThread counter reached %i, yielding thread %i on tile %i.", current_time - m_last_start_time[core_id.tile_id][thread_idx], thread_idx, core_id.tile_id);
      }
      else {
         LOG_PRINT("ThreadScheduler::yieldThread called with with ignore_timer, yielding thread %i on tile %i.", thread_idx, core_id.tile_id);
      }

      ThreadYieldRequest req = { MCP_MESSAGE_THREAD_YIELD_REQUEST,
         core_id,
         thread_idx,
         INVALID_THREAD_ID,
         INVALID_CORE_ID,
         INVALID_THREAD_ID,
         INVALID_THREAD_ID,
         is_pre_emptive
      };

      Network *net = core->getTile()->getNetwork();
      net->netSend(Config::getSingleton()->getMCPCoreId(),
            MCP_REQUEST_TYPE,
            &req,
            sizeof(req));

      NetPacket pkt = net->netRecvType(MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE, core->getId());

      m_core_lock[core_id.tile_id].acquire();

      ThreadYieldRequest * reply = (ThreadYieldRequest*) ((Byte*)pkt.data);
      core_id_t req_core_id      = reply->requester;
      thread_id_t req_thread_idx = reply->requester_tidx;
      thread_id_t req_next_tidx  = reply->requester_next_tidx;

      // Check that the received message got to the correct core and thread. 
      // I've seen problems with spurious messages coming from the MCP twice for the same thread, maybe due to some timeout handler? I wrapped this recv in a while loop.
      while(!(req_core_id.tile_id == core_id.tile_id && req_core_id.core_type == core_id.core_type && req_thread_idx == thread_idx))
      {
         m_core_lock[core_id.tile_id].release();
         NetPacket pkt = net->netRecvType(MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE, core->getId());
         m_core_lock[core_id.tile_id].acquire();

         reply = (ThreadYieldRequest*) ((Byte*)pkt.data);
         req_core_id      = reply->requester;
         __attribute__((unused)) thread_id_t req_thread_idx = reply->requester_tidx;
         req_next_tidx  = reply->requester_next_tidx;
      }

      dst_core_id    = reply->destination;
      dst_thread_idx = reply->destination_tidx;
      thread_id_t dst_next_tidx  = reply->destination_next_tidx;

      // Set next tidx on current running process.
      m_local_next_tidx[core_id.tile_id] = req_next_tidx;


      // If this thread has migrated, update local data structures.
      if(dst_core_id.tile_id != core_id.tile_id)
      {
         // Get access to the destination core's structures.
         m_core_lock[core_id.tile_id].release();
         m_core_lock[dst_core_id.tile_id].acquire();

         // Set OS TID of this thread
         m_tile_manager->updateTLS(dst_core_id.tile_id, dst_thread_idx, m_tile_manager->getCurrentThreadID());
         m_thread_manager->setOSTid(dst_core_id, dst_thread_idx, syscall(SYS_gettid));

         // If no threads are scheduled, then schedule this thread next.
         if (dst_next_tidx == INVALID_THREAD_ID)
         {
            m_local_next_tidx[dst_core_id.tile_id] = dst_thread_idx;
         }
      }
      else
      {
         m_local_next_tidx[dst_core_id.tile_id] = dst_next_tidx;
      }

      m_thread_wait_cond[req_core_id.tile_id].broadcast();

      // If this thread idx (dst_thread_idx) is not the one scheduler to run on this tile (m_local_next_tidx) then wait.
      while (dst_thread_idx != m_local_next_tidx[dst_core_id.tile_id])
      {
         m_tile_manager->getCurrentCore()->setState(Core::STALLED);
         m_thread_wait_cond[dst_core_id.tile_id].wait(m_core_lock[dst_core_id.tile_id]);
      }

      m_tile_manager->getCurrentCore()->setState(Core::RUNNING);
      m_last_start_time[dst_core_id.tile_id][dst_thread_idx] = (UInt32) time(NULL);

      LOG_PRINT("Resuming thread %i on {%i, %i}", dst_thread_idx, dst_core_id.tile_id, dst_core_id.core_type);
   }

   m_core_lock[dst_core_id.tile_id].release();
}

void ThreadScheduler::masterYieldThread(ThreadYieldRequest* req)
{
   LOG_ASSERT_ERROR(m_master, "ThreadScheduler::masterYieldThread should only be called on master.");
   core_id_t req_core_id = req->requester;
   thread_id_t requester_tidx = req->requester_tidx;
   core_id_t dst_core_id = req->requester;
   thread_id_t destination_tidx = req->requester_tidx;
   bool ignore_affinity = !req->is_pre_emptive;

   LOG_PRINT("In ThreadScheduler::masterYieldThread() for %i on {%i, %i}", requester_tidx, req_core_id.tile_id, req_core_id.core_type);


   std::vector< std::vector<ThreadManager::ThreadState> > thread_state = m_thread_manager->getThreadState();

   m_core_lock[req_core_id.tile_id].acquire();

   assert(m_waiter_queue[req_core_id.tile_id].front()->destination_tidx == requester_tidx);
   LOG_PRINT("Yielding thread %i on {%i, %i}", requester_tidx, req_core_id.tile_id, req_core_id.core_type); 

   m_thread_manager->stallThread(req_core_id, requester_tidx);

   requeueThread(req_core_id);

   bool is_thread_new = false;
   m_local_next_tidx[req_core_id.tile_id] = m_waiter_queue[req_core_id.tile_id].front()->destination_tidx;

   // Migrate this thread if it's affinity is set to another core, or if there are more threads in the
   // queue, but also cores that are idle. ie. threads get distributed.
   if (!ignore_affinity && m_thread_migration_enabled)
      masterCheckAffinityAndMigrate(req_core_id, requester_tidx, dst_core_id, destination_tidx);

   // If the queue is empty, that means all threads have moved, there is no next_tidx
   if (m_waiter_queue[req_core_id.tile_id].empty())
   {
      m_local_next_tidx[req_core_id.tile_id] = INVALID_THREAD_ID;
   }
   else
   {
      if (Tile::isMainCore(req_core_id))
         is_thread_new = m_thread_manager->isThreadInitializing(req_core_id, m_local_next_tidx[req_core_id.tile_id]);

      if (is_thread_new)
      {
         masterStartThread(req_core_id);
      }
      else
         m_thread_manager->resumeThread(req_core_id, m_local_next_tidx[req_core_id.tile_id]);
   }

   ThreadYieldRequest reply = {  MCP_MESSAGE_THREAD_YIELD_REQUEST,
                                 req_core_id,
                                 requester_tidx,
                                 m_local_next_tidx[req_core_id.tile_id],
                                 dst_core_id,
                                 destination_tidx,
                                 m_local_next_tidx[dst_core_id.tile_id]
                              };

   Core* core = m_tile_manager->getCurrentCore();
   core->getTile()->getNetwork()->netSend(req_core_id, MCP_THREAD_YIELD_REPLY_FROM_MASTER_TYPE,
                                          &reply, sizeof(reply));

   m_core_lock[req_core_id.tile_id].release();
}

void ThreadScheduler::enqueueThread(core_id_t core_id, ThreadSpawnRequest * req)
{
   ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
   *req_cpy = *req;
   m_waiter_queue[core_id.tile_id].push(req_cpy);
}

void ThreadScheduler::requeueThread(core_id_t core_id)
{
   LOG_PRINT_ERROR("No scheme was set for requeuing threads!");
}
