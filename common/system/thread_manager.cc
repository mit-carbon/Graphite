#include <sys/syscall.h>
#include "thread_manager.h"
#include "tile_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "simulator.h"
#include "mcp.h"
#include "clock_skew_minimization_object.h"
#include "network.h"
#include "message_types.h"
#include "tile.h"
#include "core.h"
#include "thread.h"
#include "packetize.h"
#include "clock_converter.h"
#include "fxsupport.h"

ThreadManager::ThreadManager(TileManager *tile_manager)
   : m_thread_spawn_sem(0)
   , m_thread_spawn_lock()
   , m_tile_manager(tile_manager)
   , m_thread_spawners_terminated(0)
{
   Config *config = Config::getSingleton();

   m_master = config->getCurrentProcessNum() == 0;
   m_enable_pep_cores = config->getEnablePepCores();

   // Set the thread-spawner and MCP tiles to running.
   if (m_master)
   {
      m_thread_state.resize(config->getTotalTiles());
      m_thread_state[0].status = Core::RUNNING;
      

      if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
      {
         // Reserve core-id's 1 to (num_processes) for thread-spawners
         UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
         UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
         for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
         {
            m_thread_state[i].status = Core::RUNNING;
         }


      }
      m_thread_state[config->getMCPTileNum()].status = Core::RUNNING;
      
      LOG_ASSERT_ERROR(config->getMCPTileNum() < (SInt32) m_thread_state.size(),
                       "MCP core num out of range (!?)");

      if(m_enable_pep_cores)
      {
         m_helper_thread_state.resize(config->getTotalTiles());
         m_helper_thread_state[0].status = Core::RUNNING;

         if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
         {
            // Reserve core-id's 1 to (num_processes) for thread-spawners
            UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
            UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
            for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
            {
               m_helper_thread_state[i].status = Core::RUNNING;
            }
         }
         m_helper_thread_state[config->getMCPTileNum()].status = Core::RUNNING;

         LOG_ASSERT_ERROR(config->getMCPTileNum() < (SInt32) m_helper_thread_state.size(),
               "MCP core num out of range (!?)");
      }
   }
}

ThreadManager::~ThreadManager()
{
   if (m_master)
   {
      m_thread_state[0].status = Core::IDLE;
      m_thread_state[Config::getSingleton()->getMCPTileNum()].status = Core::IDLE;
      LOG_ASSERT_ERROR(Config::getSingleton()->getMCPTileNum() < (SInt32)m_thread_state.size(), "MCP core num out of range (!?)");

      if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
      {
         // Reserve core-id's 1 to (num_processes) for thread-spawners
         UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
         UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
         for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
         {
            m_thread_state[i].status = Core::IDLE;
         }
      }

      for (UInt32 i = 0; i < m_thread_state.size(); i++)
         LOG_ASSERT_ERROR(m_thread_state[i].status == Core::IDLE, "Thread %d still active when ThreadManager destructs!", i);

      if(m_enable_pep_cores)
      {
         m_helper_thread_state[0].status = Core::IDLE;
         m_helper_thread_state[Config::getSingleton()->getMCPTileNum()].status = Core::IDLE;
         LOG_ASSERT_ERROR(Config::getSingleton()->getMCPTileNum() < (SInt32)m_helper_thread_state.size(), "MCP core num out of range (!?)");

         if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
         {
            // Reserve core-id's 1 to (num_processes) for thread-spawners
            UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
            UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
            for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
            {
               m_helper_thread_state[i].status = Core::IDLE;
            }
      }

      for (UInt32 i = 0; i < m_helper_thread_state.size(); i++)
         LOG_ASSERT_ERROR(m_helper_thread_state[i].status == Core::IDLE, "Thread %d still active when ThreadManager destructs!", i);

      }
   }
}

void ThreadManager::onThreadStart(ThreadSpawnRequest *req)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;
   
   LOG_PRINT("onThreadStart(Tile: %i Core: %i)", req->destination.first, req->destination.second);

   m_tile_manager->initializeThread(req->destination);

   if (req->destination.first == Sim()->getConfig()->getCurrentThreadSpawnerTileNum())
      return;

   // Set the CoreState to 'RUNNING'
   m_tile_manager->getCurrentCore()->setState(Core::RUNNING);
 
   // send ack to master
   LOG_PRINT("(5) onThreadStart -- send ack to master; req : { %p, %p, {%i, %i}, {%i, %i} }",
             req->func, req->arg, req->requester.first, req->requester.second, req->destination.first, req->destination.second);

   req->msg_type = MCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE;
   Network *net = m_tile_manager->getCurrentTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                req,
                sizeof(*req));

   CorePerfModel *pm = m_tile_manager->getCurrentCore()->getPerformanceModel();

   // Global Clock to Tile Clock
   UInt64 start_cycle_count = convertCycleCount(req->time, \
         1.0, pm->getFrequency());
   pm->queueDynamicInstruction(new SpawnInstruction(start_cycle_count));
}

void ThreadManager::onThreadExit()
{
   if (m_tile_manager->getCurrentTileID() == -1)
      return;
 
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   Tile* tile = m_tile_manager->getCurrentTile();
   Core* core = m_tile_manager->getCurrentCore();

   // send message to master process to update thread state
   SInt32 msg[] = { MCP_MESSAGE_THREAD_EXIT, m_tile_manager->getCurrentCoreID().first, m_tile_manager->getCurrentCoreID().second };

   LOG_PRINT("onThreadExit -- send message to master ThreadManager; thread {%d, %d} at time %llu",
             core->getCoreId().first, core->getCoreId().second,
             core->getPerformanceModel()->getCycleCount());
   Network *net = tile->getNetwork();

   // Set the CoreState to 'IDLE'
   core->setState(Core::IDLE);

   // Recompute Average Frequency
   core->getPerformanceModel()->recomputeAverageFrequency();

   // terminate thread locally so we are ready for new thread requests
   // on that tile
   m_tile_manager->terminateThread();

   // update global thread state
   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                msg,
                sizeof(SInt32) + sizeof(core_id_t));
}

void ThreadManager::masterOnThreadExit(tile_id_t tile_id, UInt32 core_type,  UInt64 time)
{
   LOG_ASSERT_ERROR(m_master, "masterOnThreadExit should only be called on master.");
   LOG_PRINT("masterOnThreadExit : {%d, %d}", tile_id, core_type);
   
   if (core_type == PEP_CORE_TYPE)
   {
      LOG_ASSERT_ERROR((UInt32)tile_id < m_helper_thread_state.size(), "Tile id out of range: %d", tile_id);
      assert(m_helper_thread_state[tile_id].status == Core::RUNNING);
      m_helper_thread_state[tile_id].status = Core::IDLE;
   }
   else
   {
      LOG_ASSERT_ERROR((UInt32)tile_id < m_thread_state.size(), "Tile id out of range: %d", tile_id);
      assert(m_thread_state[tile_id].status == Core::RUNNING);
      m_thread_state[tile_id].status = Core::IDLE;
   }
   
   if (Sim()->getMCP()->getClockSkewMinimizationServer())
      Sim()->getMCP()->getClockSkewMinimizationServer()->signal();

   wakeUpWaiter((core_id_t) {tile_id, core_type}, time);

   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
      slaveTerminateThreadSpawnerAck(tile_id);
}

/*
  Thread spawning occurs in the following steps:

  1. A message is sent from requestor to the master thread manager.
  2. The master thread manager finds the destination core and sends a message to its host process.
  3. The host process spawns the new thread by adding the information to a list and posting a sempaphore.
  4. The thread is spawned by the thread spawner coming from the application.
  5. The spawned thread initializes and then sends an ACK to the master thread manager.
  6. The master thread manager replies to the requestor with the id of the dest core.
*/

SInt32 ThreadManager::spawnThread(thread_func_t func, void *arg)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // step 1
   LOG_PRINT("(1) spawnThread with func: %p and arg: %p", func, arg);

   Core *core = m_tile_manager->getCurrentCore();
   Network *net = core->getNetwork();

   // Tile Clock to Global Clock
   UInt64 global_cycle_count = convertCycleCount(core->getPerformanceModel()->getCycleCount(), \
         core->getPerformanceModel()->getFrequency(), 1.0);

   ThreadSpawnRequest req = { MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER,
                              func, arg, core->getCoreId(), INVALID_CORE_ID,
                              global_cycle_count };

   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   // Set the CoreState to 'STALLED'
   core->setState(Core::STALLED);

   NetPacket pkt = net->netRecvType(MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE);
   
   LOG_ASSERT_ERROR(pkt.length == sizeof(core_id_t), "Unexpected reply size.");

   // Set the CoreState to 'RUNNING'
   core->setState(Core::RUNNING);

   core_id_t core_id = *((core_id_t*)pkt.data);
   LOG_PRINT("Thread spawned on core: {%d, %d}", core_id.first, core_id.second);

   // Delete the data buffer
   delete [] (Byte*) pkt.data;

   // elau: Return the tile_id...treated as thread id?  Might crash.
   return core_id.first;
}

SInt32 ThreadManager::spawnHelperThread(thread_func_t func, void *arg)
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // step 1
   LOG_PRINT("(1) spawnHelperThread with func: %p and arg: %p", func, arg);

   Core *core = m_tile_manager->getCurrentCore();
   Network *net = core->getNetwork();

   // Tile Clock to Global Clock
   UInt64 global_cycle_count = convertCycleCount(core->getPerformanceModel()->getCycleCount(), \
         core->getPerformanceModel()->getFrequency(), 1.0);

   // The request is to spawn this new thread to the same tile.
   ThreadSpawnRequest req = { MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER,
                              func, arg, core->getCoreId(), (core_id_t) {core->getCoreId().first, PEP_CORE_TYPE},
                              global_cycle_count };

   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   // Set the CoreState to 'STALLED'
   core->setState(Core::STALLED);

   NetPacket pkt = net->netRecvType(MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE);
   
   LOG_ASSERT_ERROR(pkt.length == sizeof(core_id_t), "Unexpected reply size.");

   // Set the CoreState to 'RUNNING'
   core->setState(Core::RUNNING);

   core_id_t core_id = *((core_id_t*)pkt.data);
   LOG_PRINT("Thread spawned on core: {%d, %d}", core_id.first, core_id.second);

   // Delete the data buffer
   delete [] (Byte*) pkt.data;

   return core_id.first;
}


void ThreadManager::masterSpawnThread(ThreadSpawnRequest *req)
{
   // step 2
   LOG_ASSERT_ERROR(m_master, "masterSpawnThread should only be called on master.");
   LOG_PRINT("(2) masterSpawnThread with req: { %p, %p, {%d, %d}, {%d, %d} }", req->func, req->arg, req->requester.first, req->requester.second, req->destination.first, req->destination.second);

   // find core to use
   // FIXME: Load balancing?
   if (req->destination.second == PEP_CORE_TYPE)
   {
      for (SInt32 i = 0; i < (SInt32) m_helper_thread_state.size(); i++)
      {
         if (m_helper_thread_state[i].status == Core::IDLE)
         {
            req->destination = (core_id_t) {i, PEP_CORE_TYPE};
            break;
         }
      }
   }
   else
   {
      for (SInt32 i = 0; i < (SInt32) m_thread_state.size(); i++)
      {
         if (m_thread_state[i].status == Core::IDLE)
         {
            req->destination = (core_id_t) {i, MAIN_CORE_TYPE};
            break;
         }
      }
   }

   LOG_ASSERT_ERROR(req->destination.first != INVALID_TILE_ID, "No cores available for spawnThread request.");

   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {  
      // Mark the requesting thread as stalled
      stallThread(req->requester);
       
      // spawn process on child
      SInt32 dest_proc = Config::getSingleton()->getProcessNumForTile(req->destination.first);
      Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

      req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_MASTER;

      globalNode->globalSend(dest_proc, req, sizeof(*req));
      
      LOG_ASSERT_ERROR((UInt32)req->destination.first < m_thread_state.size(), "Tile id out of range: %d", req->destination.first);
   }
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
   {
      LOG_PRINT("New Thread to be spawned with core id(%i, %i)", req->destination.first, req->destination.second);
      ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
      *req_cpy = *req;
      
      // Insert the request in the thread request queue and
      // the thread request map
      insertThreadSpawnRequest(req_cpy);
      m_thread_spawn_sem.signal();
         
      Core *core = m_tile_manager->getCurrentCore();
      core->getNetwork()->netSend(req->requester.first, 
            MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE,
            &req->destination,
            sizeof(req->destination));
   }

   LOG_PRINT("Setting status[%i, %i] -> INITIALIZING", req->destination.first, req->destination.second);

   if (req->destination.second == PEP_CORE_TYPE)
      m_helper_thread_state[req->destination.first].status = Core::INITIALIZING;
   else
      m_thread_state[req->destination.first].status = Core::INITIALIZING;

   LOG_PRINT("Done with (2)");
}

void ThreadManager::slaveSpawnThread(ThreadSpawnRequest *req)
{
   // step 3
   LOG_PRINT("(3) slaveSpawnThread with req: { fun: %p, arg: %p, req: {%d, %d}, core: {%d, %d} }", req->func, req->arg, req->requester.first, req->requester.second, req->destination.first, req->destination.second);

   // This is deleted after the thread has been spawned
   // and we have sent the acknowledgement back to the requester
   ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
   *req_cpy = *req;

   // Insert the request in the thread request queue and
   // the thread request map
   insertThreadSpawnRequest (req_cpy);
   m_thread_spawn_sem.signal();
}

void ThreadManager::getThreadToSpawn(ThreadSpawnRequest *req)
{
   // step 4 - this is called from the thread spawner
   LOG_PRINT("(4a) getThreadToSpawn called by user.");
   
   // Wait for a request to arrive
   m_thread_spawn_sem.wait();
   
   // Grab the request and set the argument
   // The lock is released by the spawned thread
   m_thread_spawn_lock.acquire();
   *req = *((ThreadSpawnRequest*) m_thread_spawn_list.front());
   
   LOG_PRINT("(4b) getThreadToSpawn giving thread %p arg: %p to user.", req->func, req->arg);
}

ThreadSpawnRequest* ThreadManager::getThreadSpawnReq()
{
   if (m_thread_spawn_list.empty())
   {
      return (ThreadSpawnRequest*) NULL;
   }
   else
   {
      return m_thread_spawn_list.front();
   }
}

void ThreadManager::dequeueThreadSpawnReq (ThreadSpawnRequest *req)
{
   ThreadSpawnRequest *thread_req = m_thread_spawn_list.front();

   *req = *thread_req;

   m_thread_spawn_list.pop();
   m_thread_spawn_lock.release();

   delete thread_req;

   LOG_PRINT("Dequeued req: { %p, %p, {%d, %d}, {%d, %d} }", req->func, req->arg, req->requester.first, req->requester.second, req->destination.first, req->destination.second);
}

void ThreadManager::masterSpawnThreadReply(ThreadSpawnRequest *req)
{
   // step 6
   LOG_ASSERT_ERROR(m_master, "masterSpawnThreadReply should only be called on master.");
   LOG_PRINT("(6) masterSpawnThreadReply with req: { fun: %p, arg: %p, req: {%d, %d} core: {%d, %d} }", req->func, req->arg, req->requester.first, req->requester.second, req->destination.first, req->destination.second);

   // Set the state of the actual thread spawned to running
   LOG_PRINT("Setting status[%i, %i] -> RUNNING", req->destination.first, req->destination.second);

   if (req->destination.second == PEP_CORE_TYPE)
      m_helper_thread_state[req->destination.first].status = Core::RUNNING;
   else
      m_thread_state[req->destination.first].status = Core::RUNNING;

   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {
      // Resume the requesting thread
      resumeThread(req->requester);

      Core *core = m_tile_manager->getCurrentCore();
      core->getNetwork()->netSend(req->requester.first, 
                                  MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE,
                                  &req->destination,
                                  sizeof(req->destination));
   }
}

bool ThreadManager::areAllCoresRunning()
{
   LOG_ASSERT_ERROR(m_master, "areAllCoresRunning() should only be called on master.");
   // Check if all the cores are running
   bool is_all_running = true;
   for (SInt32 i = 0; i < (SInt32) m_thread_state.size(); i++)
   {
      if (m_thread_state[i].status == Core::IDLE)
      {
         is_all_running = false;
         break;
      }
   }

   return is_all_running;
}

void ThreadManager::joinThread(tile_id_t tile_id)
{
   // joinThread joins the current thread with the MAIN core's thread at tile_id. 
   LOG_PRINT("Joining main thread on tile: %d", tile_id);

   // Send the message to the master process; will get reply when thread is finished
   ThreadJoinRequest msg = { MCP_MESSAGE_THREAD_JOIN_REQUEST,
                             m_tile_manager->getCurrentCoreID(),
                             (core_id_t) {tile_id, MAIN_CORE_TYPE} };

   Network *net = m_tile_manager->getCurrentTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                &msg,
                sizeof(msg));

   // Set the CoreState to 'STALLED'
   m_tile_manager->getCurrentCore()->setState(Core::STALLED);

   // Wait for reply
   NetPacket pkt = net->netRecvType(MCP_THREAD_JOIN_REPLY);

   // Set the CoreState to 'WAKING_UP'
   m_tile_manager->getCurrentCore()->setState(Core::WAKING_UP);

   LOG_PRINT("Exiting join main thread.");
}

void ThreadManager::joinHelperThread(tile_id_t tile_id)
{
   // joinHelperThread joins the current thread with the PEP core's thread at tile_id. 
   LOG_PRINT("Joining helper thread on tile: %d", tile_id);

   // Send the message to the master process; will get reply when thread is finished
   ThreadJoinRequest msg = { MCP_MESSAGE_THREAD_JOIN_REQUEST,
                             m_tile_manager->getCurrentCoreID(),
                             (core_id_t) {tile_id, PEP_CORE_TYPE} };

   Network *net = m_tile_manager->getCurrentTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPTileNum(),
                MCP_REQUEST_TYPE,
                &msg,
                sizeof(msg));

   // Set the CoreState to 'STALLED'
   m_tile_manager->getCurrentCore()->setState(Core::STALLED);

   // Wait for reply
   NetPacket pkt = net->netRecvType(MCP_THREAD_JOIN_REPLY);

   // Set the CoreState to 'WAKING_UP'
   m_tile_manager->getCurrentCore()->setState(Core::WAKING_UP);

   LOG_PRINT("Exiting join helper thread.");
}

void ThreadManager::masterJoinThread(ThreadJoinRequest *req, UInt64 time)
{
   LOG_ASSERT_ERROR(m_master, "masterJoinThread should only be called on master.");

   LOG_PRINT("masterJoinThread called on core: {%d, %d}", req->receiver.first, req->receiver.second);
   //FIXME: fill in the proper time

   if (req->receiver.second == PEP_CORE_TYPE)
   {
      LOG_ASSERT_ERROR(m_helper_thread_state[req->receiver.first].waiter.first == INVALID_TILE_ID,
            "Multiple threads joining on helper thread on tile %d", req->receiver.first);

      m_helper_thread_state[req->receiver.first].waiter = req->sender;

      // Stall the 'pthread_join' caller
      stallThread(req->sender);

      // Tile not running, so the thread must have joined
      LOG_ASSERT_ERROR((UInt32)req->receiver.first < m_helper_thread_state.size(), "Tile id out of range: %d", req->receiver.first);
      if (m_helper_thread_state[req->receiver.first].status == Core::IDLE)
      {
         LOG_PRINT("Not running, sending reply.");
         wakeUpWaiter(req->receiver, time);
      }
   }
   else
   {
      LOG_ASSERT_ERROR(m_thread_state[req->receiver.first].waiter.first == INVALID_TILE_ID,
            "Multiple threads joining on main thread on tile %d", req->receiver.first);

      m_thread_state[req->receiver.first].waiter = req->sender;

      // Stall the 'pthread_join' caller
      stallThread(req->sender);

      // Tile not running, so the thread must have joined
      LOG_ASSERT_ERROR((UInt32)req->receiver.first < m_thread_state.size(), "Tile id out of range: %d", req->receiver.first);

      if (m_thread_state[req->receiver.first].status == Core::IDLE)
      {
         LOG_PRINT("Not running, sending reply.");
         wakeUpWaiter(req->receiver, time);
      }
   }
}

void ThreadManager::wakeUpWaiter(core_id_t core_id, UInt64 time)
{
   if (core_id.second == PEP_CORE_TYPE)
      wakeUpHelperWaiter(core_id, time);
   else if (core_id.second == MAIN_CORE_TYPE)
      wakeUpMainWaiter(core_id, time);
   else
      LOG_ASSERT_ERROR(false, "Unrecognized core type to wake up");
}

void ThreadManager::wakeUpMainWaiter(core_id_t core_id, UInt64 time)
{
   LOG_ASSERT_ERROR(core_id.second == MAIN_CORE_TYPE, "wakeUpWaiter is for threads waiting on main threads only!");
   if (m_thread_state[core_id.first].waiter.first != INVALID_TILE_ID)
   {
      LOG_PRINT("Waking up core: {%d, %d} at time: %llu", m_thread_state[core_id.first].waiter.first, m_thread_state[core_id.first].waiter.second, time);

      Core *core = m_tile_manager->getCurrentCore();
      core_id_t dest = m_thread_state[core_id.first].waiter;

      // Resume the 'pthread_join' caller
      resumeThread(dest);

      // we have to manually send a packet because we are
      // manufacturing a time stamp
      NetPacket pkt(time,
                    MCP_THREAD_JOIN_REPLY,
                    core->getTileId(),
                    dest.first,
                    0,
                    NULL);
      core->getNetwork()->netSend(pkt);

      m_thread_state[core_id.first].waiter = INVALID_CORE_ID;
   }
   LOG_PRINT("Exiting wakeUpMainWaiter");
}

void ThreadManager::wakeUpHelperWaiter(core_id_t core_id, UInt64 time)
{
   LOG_ASSERT_ERROR(core_id.second == PEP_CORE_TYPE, "wakeUpHelperWaiter is for threads waiting on helper threads only!");
   if (m_helper_thread_state[core_id.first].waiter.first != INVALID_TILE_ID)
   {
      LOG_PRINT("Waking up core: {%d, %d} at time: %llu", m_helper_thread_state[core_id.first].waiter.first, m_helper_thread_state[core_id.first].waiter.second, time);

      Core *core = m_tile_manager->getCurrentCore();
      core_id_t dest = m_helper_thread_state[core_id.first].waiter;

      // Resume the 'pthread_join' caller
      resumeThread(dest);

      // we have to manually send a packet because we are
      // manufacturing a time stamp
      NetPacket pkt(time,
                    MCP_THREAD_JOIN_REPLY,
                    core->getTileId(),
                    dest.first,
                    0,
                    NULL);
      core->getNetwork()->netSend(pkt);

      m_helper_thread_state[core_id.first].waiter = INVALID_CORE_ID;
   }
   LOG_PRINT("Exiting wakeUpHelperWaiter");
}

void ThreadManager::insertThreadSpawnRequest(ThreadSpawnRequest *req)
{
   // Insert the request in the thread request queue
   m_thread_spawn_lock.acquire();
   m_thread_spawn_list.push(req);
   m_thread_spawn_lock.release();
}

void ThreadManager::terminateThreadSpawners()
{
   LOG_PRINT ("In terminateThreadSpawner");

   Transport::Node *node = Transport::getSingleton()->getGlobalNode();

   assert(node);

   for (UInt32 pid = 0; pid < Config::getSingleton()->getProcessCount(); pid++)
   {
      int send_req = LCP_MESSAGE_QUIT_THREAD_SPAWNER;
      
      LOG_PRINT ("Sending thread spawner quit message to proc %d", pid);
      node->globalSend (pid, &send_req, sizeof (send_req));
      LOG_PRINT ("Sent thread spawner quit message to proc %d", pid);
   }

   // wait for all thread spawners to terminate
   while (true)
   {
      {
         ScopedLock sl(m_thread_spawners_terminated_lock);
         if (m_thread_spawners_terminated < Config::getSingleton()->getProcessCount())
            break;
      }
      sched_yield();
   }
}

void ThreadManager::slaveTerminateThreadSpawner()
{
   LOG_PRINT ("slaveTerminateThreadSpawner on proc %d", Config::getSingleton()->getCurrentProcessNum());

   ThreadSpawnRequest *req = new ThreadSpawnRequest;

   req->msg_type = LCP_MESSAGE_QUIT_THREAD_SPAWNER;
   req->func = NULL;
   req->arg = NULL;
   req->requester = INVALID_CORE_ID;
   req->destination = INVALID_CORE_ID;

   insertThreadSpawnRequest(req);
   m_thread_spawn_sem.signal();
}

void ThreadManager::slaveTerminateThreadSpawnerAck(tile_id_t tile_id)
{
   Config *config = Config::getSingleton();
   for (UInt32 i = 0; i < config->getProcessCount(); i++)
   {
      if (tile_id == config->getThreadSpawnerTileNum(i))
      {
         Transport::Node *node = m_tile_manager->getCurrentTile()->getNetwork()->getTransport();

         int req_type = LCP_MESSAGE_QUIT_THREAD_SPAWNER_ACK;

         node->globalSend(0, &req_type, sizeof(req_type));
      }
   }
}

void ThreadManager::updateTerminateThreadSpawner()
{
   assert(Config::getSingleton()->getCurrentProcessNum() == 0);
   ScopedLock sl(m_thread_spawners_terminated_lock);
   ++m_thread_spawners_terminated;
}

void ThreadManager::stallThread(core_id_t core_id)
{
   LOG_PRINT("Core(%i, %i) -> STALLED", core_id.first, core_id.second);
   LOG_ASSERT_ERROR(m_master, "stallThread() must only be called on master");

   if (core_id.second == PEP_CORE_TYPE)
      stallHelperThread(core_id.first);
   else
      stallThread(core_id.first);
}

void ThreadManager::stallThread(tile_id_t tile_id)
{
   m_thread_state[tile_id].status = Core::STALLED;
   
   if (Sim()->getMCP()->getClockSkewMinimizationServer())
      Sim()->getMCP()->getClockSkewMinimizationServer()->signal();
}

void ThreadManager::stallHelperThread(tile_id_t tile_id)
{
   m_helper_thread_state[tile_id].status = Core::STALLED;
   
   if (Sim()->getMCP()->getClockSkewMinimizationServer())
      Sim()->getMCP()->getClockSkewMinimizationServer()->signal();
}

void ThreadManager::resumeThread(core_id_t core_id)
{
   LOG_PRINT("Core(%i, %i) -> RUNNING", core_id.first, core_id.second);
   LOG_ASSERT_ERROR(m_master, "resumeThread() must only be called on master");

   if (core_id.second == PEP_CORE_TYPE)
      resumeHelperThread(core_id.first);
   else
      resumeThread(core_id.first);
}

void ThreadManager::resumeThread(tile_id_t tile_id)
{
   m_thread_state[tile_id].status = Core::RUNNING;
}

void ThreadManager::resumeHelperThread(tile_id_t tile_id)
{
   m_helper_thread_state[tile_id].status = Core::RUNNING;
}

bool ThreadManager::isThreadRunning(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "isThreadRunning() must only be called on master");

   if (core_id.second == PEP_CORE_TYPE)
      return isHelperThreadRunning(core_id.first);
   else
      return isThreadRunning(core_id.first);
}

bool ThreadManager::isThreadRunning(tile_id_t tile_id)
{
   return (m_thread_state[tile_id].status == Core::RUNNING);
}

bool ThreadManager::isHelperThreadRunning(tile_id_t tile_id)
{
   return (m_helper_thread_state[tile_id].status == Core::RUNNING);
}

bool ThreadManager::isThreadInitializing(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "isThreadInitializing() must only be called on master");
   if (core_id.second == PEP_CORE_TYPE)
      return isHelperThreadInitializing(core_id.first);
   else
      return isThreadInitializing(core_id.first);
}

bool ThreadManager::isThreadInitializing(tile_id_t tile_id)
{
   return (m_thread_state[tile_id].status == Core::INITIALIZING);
}

bool ThreadManager::isHelperThreadInitializing(tile_id_t tile_id)
{
   return (m_helper_thread_state[tile_id].status == Core::INITIALIZING);
}

