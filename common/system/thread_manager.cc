#include <sys/syscall.h>
#include <cassert>
#include "thread_manager.h"
#include "thread_scheduler.h"
#include "tile_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "simulator.h"
#include "mcp.h"
#include "clock_skew_management_object.h"
#include "network.h"
#include "message_types.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "thread.h"
#include "packetize.h"

ThreadManager::ThreadManager(TileManager *tile_manager)
   : m_thread_spawn_sem(0)
   , m_thread_spawn_lock()
   , m_tile_manager(tile_manager)
   , m_thread_spawners_terminated(0)
{
   Config *config = Config::getSingleton();

   m_master = config->getCurrentProcessNum() == 0;

   m_tid_counter = 0;

   // Set the thread-spawner and MCP tiles to running.
   if (m_master)
   {
      UInt32 total_tiles = config->getTotalTiles();
      UInt32 threads_per_core = config->getMaxThreadsPerCore();

      m_thread_state.resize(total_tiles);
      for (UInt32 i = 0; i < total_tiles; i++)
         m_thread_state[i].resize(threads_per_core);

      // Initialize all threads to IDLE and zero affinity masks
      for (UInt32 i = 0; i < total_tiles; i++) {
         for (UInt32 j = 0; j < threads_per_core; j++) {
            m_thread_state[i][j].status = Core::IDLE;
            m_thread_state[i][j].cpu_set = CPU_ALLOC(total_tiles);
            CPU_ZERO_S(CPU_ALLOC_SIZE(total_tiles), m_thread_state[i][j].cpu_set);
         }
      }

      m_thread_state[0][0].status = Core::RUNNING;
            
      m_last_stalled_thread.resize(total_tiles);
      for (UInt32 i = 0; i < total_tiles; i++)
         m_last_stalled_thread[i] = INVALID_THREAD_ID;

      if (config->getSimulationMode() == Config::FULL)
      {
         UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
         UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
         for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
         {
            m_thread_state[i][0].status = Core::RUNNING;
         }
      }
      m_thread_state[config->getMCPTileNum()][0].status = Core::RUNNING;
      
      LOG_ASSERT_ERROR(config->getMCPTileNum() < (SInt32) m_thread_state.size(),
                       "MCP core num out of range (!?)");
   }
}

ThreadManager::~ThreadManager()
{
   if (m_master)
   {
      m_thread_state[0][0].status = Core::IDLE;
      m_thread_state[Config::getSingleton()->getMCPTileNum()][0].status = Core::IDLE;
      LOG_ASSERT_ERROR(Config::getSingleton()->getMCPTileNum() < (SInt32)m_thread_state.size(), "MCP core num out of range (!?)");

      if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
      {
         // Reserve core-id's 1 to (num_processes) for thread-spawners
         UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - Sim()->getConfig()->getProcessCount() - 1;
         UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalTiles() - 2;
         for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
         {
            m_thread_state[i][0].status = Core::IDLE;
         }
      }
      
      for (UInt32 i = 0; i < Sim()->getConfig()->getApplicationTiles(); i++)
         for (UInt32 j = 0; j < Sim()->getConfig()->getMaxThreadsPerCore(); j++)
         {
            LOG_ASSERT_ERROR(m_thread_state[i][j].status == Core::IDLE, "Main thread %d on tile %d still active when ThreadManager destructs!", j, i);
            CPU_FREE(m_thread_state[i][j].cpu_set);
         }
   }
}

void ThreadManager::onThreadStart(ThreadSpawnRequest *req)
{
   LOG_PRINT("onThreadStart[Core: (%i,%i) Thread: %i tid: %i]",
         req->destination.tile_id, req->destination.core_type, req->destination_tidx, req->destination_tid);

   m_tile_manager->initializeThread(req->destination, req->destination_tidx, req->destination_tid);
   assert(req->destination.tile_id == m_tile_manager->getCurrentCoreID().tile_id &&
          req->destination.core_type == m_tile_manager->getCurrentCoreID().core_type &&
          req->destination_tidx == m_tile_manager->getCurrentThreadIndex() &&
          req->destination_tid == m_tile_manager->getCurrentThreadID());

   if (req->destination.tile_id == Config::getSingleton()->getCurrentThreadSpawnerTileNum())
      return;

   Core* core = m_tile_manager->getCurrentCore();
   assert(core->getState() == Core::IDLE || core->getState() == Core::STALLED);

   // Set OS-TID (operating system - thread ID) of this thread
   setOSTid(req->destination, req->destination_tidx, syscall(SYS_gettid));

   // Set the CoreState to 'RUNNING'
   core->setState(Core::RUNNING);

   // send message to master process to update global thread state 
   Network *net = core->getTile()->getNetwork();
   SInt32 msg[] = { MCP_MESSAGE_THREAD_START, core->getId().tile_id, core->getId().core_type, m_tile_manager->getCurrentThreadIndex()};
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                msg,
                sizeof(SInt32) + sizeof(core_id_t) + sizeof(thread_id_t));

   CoreModel *core_model = core->getModel();
   if (core_model)
   {
      core_model->processDynamicInstruction(new SpawnInstruction(Time(req->time)));
   }
}

void ThreadManager::masterOnThreadStart(tile_id_t tile_id, UInt32 core_type, SInt32 thread_idx)
{
   // Set the CoreState to 'RUNNING'
   LOG_ASSERT_ERROR(m_thread_state[tile_id][thread_idx].status == Core::INITIALIZING,
         "Main thread should be in initializing state but isn't (state = %i)", m_thread_state[tile_id][thread_idx].status);
   m_thread_state[tile_id][thread_idx].status = Core::RUNNING;
}

void ThreadManager::onThreadExit()
{
   if (m_tile_manager->getCurrentTileID() == -1)
      return;
 
   Core* core = m_tile_manager->getCurrentCore();
   thread_id_t thread_idx = m_tile_manager->getCurrentThreadIndex();

   LOG_PRINT("onThreadExit -- send message to master ThreadManager; thread on core ID (%d,%d) IDX (%d) at time %llu",
             core->getId().tile_id, core->getId().core_type, thread_idx,
             core->getModel()->getCurrTime().toNanosec());
   Network *net = core->getTile()->getNetwork();

   // Recompute Average Frequency
   CoreModel* core_model = core->getModel();
   core_model->recomputeAverageFrequency(core->getFrequency());

   // send message to master process to update thread state
   SInt32 msg[] = { MCP_MESSAGE_THREAD_EXIT, core->getId().tile_id, core->getId().core_type, thread_idx };

   // update global thread state
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                msg,
                sizeof(SInt32) + sizeof(core_id_t) + sizeof(thread_id_t));

   m_thread_scheduler->onThreadExit();

   // Set the CoreState to 'IDLE'
   core->setState(Core::IDLE);

   // Terminate thread spawners if thread '0'
   if ((core->getId().tile_id == 0) && (Config::getSingleton()->getSimulationMode() == Config::FULL))
   {
      LOG_ASSERT_ERROR(Config::getSingleton()->getCurrentProcessNum() == 0, "Curr Process Num(%d)",
                       Config::getSingleton()->getCurrentProcessNum());
      terminateThreadSpawners();
   }

   // terminate thread locally so we are ready for new thread requests on that tile
   m_tile_manager->terminateThread();

   LOG_PRINT("Finished onThreadExit: thread on core ID(%d,%d), IDX(%d)", core->getId().tile_id, core->getId().core_type, thread_idx);
}

void ThreadManager::masterOnThreadExit(tile_id_t tile_id, UInt32 core_type, SInt32 thread_idx, UInt64 time)
{
   LOG_ASSERT_ERROR(m_master, "masterOnThreadExit should only be called on master.");
   core_id_t core_id = (core_id_t) {tile_id, core_type};
   
   LOG_PRINT("masterOnThreadExit: thread on core ID(%d,%d) IDX(%i)", tile_id, core_type, thread_idx);
   LOG_ASSERT_ERROR((UInt32) tile_id < m_thread_state.size(), "Tile ID (%d) out of range", tile_id);
   LOG_ASSERT_ERROR(m_thread_state[tile_id][thread_idx].status == Core::RUNNING,
         "Exiting: thread on core ID(%d,%d), IDX(%d) is NOT running", tile_id, core_type, thread_idx);
   m_thread_state[tile_id][thread_idx].status = Core::IDLE;
   m_thread_state[tile_id][thread_idx].completion_time = Time(time);

   if (Sim()->getMCP()->getClockSkewManagementServer())
      Sim()->getMCP()->getClockSkewManagementServer()->signal();

   __attribute__((unused)) bool woken_up = wakeUpWaiter(core_id, thread_idx, Time(time));

   m_thread_scheduler->masterOnThreadExit(core_id, thread_idx);

   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
      slaveTerminateThreadSpawnerAck(tile_id);
}

/*
  Thread spawning occurs in the following steps:

  1. A message is sent from requestor to the master thread manager.
  2. The master thread manager finds the destination core and asks the master thread scheduler to schedule the thread.
  3. The master thread scheduler enters the thread into a queue, and decides if the core is idle and contacts the host process if so.
  4. The master thread manager sends an ACK back to the thread that called for the new thread, and it replies with the tid of the newly spawned thread.
  5. The host process spawns the new thread by adding the information to a list and posting a sempaphore.
  6. The thread is spawned by the thread spawner coming from the application.
  7. The spawned thread is picked up by the local thread spawner and initializes.
*/

SInt32 ThreadManager::spawnThread(tile_id_t dest_tile_id, thread_func_t func, void *arg)
{
   // step 1
   LOG_PRINT("(1) spawnThread with func: %p and arg: %p", func, arg);

   Core *core = m_tile_manager->getCurrentCore();
   thread_id_t thread_index = m_tile_manager->getCurrentThreadIndex();
   Network *net = core->getTile()->getNetwork();

   Time curr_time = core->getModel()->getCurrTime();
   
   core->setState(Core::STALLED);

   core_id_t dest_core_id = INVALID_CORE_ID;
   // If destination was specified, send it there.  Otherwise pick a free core.
   if (dest_tile_id != INVALID_TILE_ID)
      dest_core_id = Tile::getMainCoreId(dest_tile_id);

   ThreadSpawnRequest req = { MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER,
                              func, arg,
                              core->getId(), thread_index,
                              dest_core_id, INVALID_THREAD_ID, INVALID_THREAD_ID,
                              curr_time.getTime() };

   core_id_t mcp_core_id = Config::getSingleton()->getMCPCoreId();
   net->netSend(mcp_core_id,
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   NetPacket pkt = net->netRecvType(MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE, core->getId());
   
   LOG_ASSERT_ERROR(pkt.length == (sizeof(core_id_t) + sizeof(thread_id_t) + sizeof(thread_id_t)),
         "Unexpected reply size: pkt_length(%u), expected(%u)",
         pkt.length, sizeof(core_id_t) + sizeof(thread_id_t) + sizeof(thread_id_t));

   // Set the CoreState to 'RUNNING'
   core->setState(Core::RUNNING);

   dest_core_id = *(core_id_t*)((Byte*) pkt.data);
   dest_tile_id = dest_core_id.tile_id;
   __attribute__((unused)) thread_id_t dest_thread_index = *(thread_id_t*) ((Byte*) pkt.data + sizeof(core_id_t));
   thread_id_t dest_thread_id = *(thread_id_t*) ((Byte*) pkt.data + sizeof(core_id_t) + sizeof(thread_id_t));
   LOG_PRINT("Thread: %i spawned on core(%d, %d), idx(%i)", dest_thread_id, dest_tile_id, dest_core_id.core_type, dest_thread_index);

   // Delete the data buffer
   delete [] (Byte*) pkt.data;

   return dest_thread_id;
}

void ThreadManager::masterSpawnThread(ThreadSpawnRequest *req)
{
   // step 2
   LOG_ASSERT_ERROR(m_master, "masterSpawnThread should only be called on master.");
   LOG_PRINT("(2) masterSpawnThread with req: { %p, %p, (%d, %d), (%d, %d) }",
         req->func, req->arg,
         req->requester.tile_id, req->requester.core_type,
         req->destination.tile_id, req->destination.core_type);

   // find core to use
   // FIXME: Load balancing?
   Config * config = Config::getSingleton();
   tile_id_t target_tile = req->requester.tile_id;
   UInt32 num_application_tiles = config->getApplicationTiles();
   stallThread(req->requester, req->requester_tidx);

   if (req->destination.tile_id == INVALID_TILE_ID)
   {
      // Threads are strided across cores.
      for (SInt32 j = 0; j < (SInt32) config->getMaxThreadsPerCore(); j++)
      {
         for (SInt32 i = 1; i <= (SInt32) num_application_tiles; i++)
         {
            target_tile = (req->requester.tile_id + i) % num_application_tiles;

            // FIXME: Core 0 can't be multithreaded.
            if (target_tile == 0)
               continue;

            if (m_thread_state[target_tile][j].status == Core::IDLE)
            {
               req->destination =  Tile::getMainCoreId(target_tile);
               req->destination_tidx =  j;
               break;
            }
         }
      }
   }
   else
   {
      // Find a free thread.
      req->destination_tidx = getIdleThread(req->destination);
   }


   LOG_ASSERT_ERROR(req->destination.tile_id != INVALID_TILE_ID, "No cores available for spawnThread request.");
   LOG_ASSERT_ERROR(req->destination_tidx != INVALID_THREAD_ID, "No threads available on destination core for spawnThread request.");

   req->destination_tid = getNewThreadId(req->destination, req->destination_tidx);
   LOG_ASSERT_ERROR(req->destination_tid != INVALID_THREAD_ID, "Problem generating new thread id.");

   m_thread_scheduler->masterScheduleThread(req);

   // Tell the spawning thread we are finished.
   LOG_PRINT("masterSpawnThread -- send ack to master: func(%p), arg(%p), "
             "Requester[core ID(%i, %i), thread IDX(%i)], Destination[core ID(%i, %i), thread IDX(%i), thread ID(%i)]",
             req->func, req->arg, req->requester.tile_id, req->requester.core_type, req->requester_tidx,
             req->destination.tile_id, req->destination.core_type, req->destination_tidx, req->destination_tid);

   masterSpawnThreadReply(req);
}

void ThreadManager::slaveSpawnThread(ThreadSpawnRequest *req)
{
   LOG_PRINT("(5) slaveSpawnThread with req: { fun: %p, arg: %p, req: (%d, %d), req thread: %i, dst: {%d, %d}, dst thread: %i destination_tid: %i }", req->func, req->arg, req->requester.tile_id, req->requester.core_type, req->requester_tidx, req->destination.tile_id, req->destination.core_type, req->destination_tidx, req->destination_tid);

   // This is deleted after the thread has been spawned and we have sent the acknowledgement back to the requester
   ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
   *req_cpy = *req;

   // Insert the request in the thread request queue and the thread request map
   insertThreadSpawnRequest (req_cpy);

   m_thread_spawn_sem.signal();
}

void ThreadManager::getThreadToSpawn(ThreadSpawnRequest *req)
{
   // step 6 - this is called from the thread spawner
   LOG_PRINT("(6a) getThreadToSpawn called by user.");
   
   // Wait for a request to arrive
   m_thread_spawn_sem.wait();
   
   // Grab the request and set the argument
   // The lock is released by the spawned thread
   m_thread_spawn_lock.acquire();
   *req = *((ThreadSpawnRequest*) m_thread_spawn_list.front());
   
   LOG_PRINT("(6b) getThreadToSpawn giving thread %p arg: %p to user.", req->func, req->arg);
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

   LOG_PRINT("Dequeued req: { %p, %p, {%d, %d}, {%d, %d} %i }", req->func, req->arg, req->requester.tile_id, req->requester.core_type, req->destination.tile_id, req->destination.core_type, req->destination_tidx);
}

void ThreadManager::masterSpawnThreadReply(ThreadSpawnRequest *req)
{
   LOG_ASSERT_ERROR(m_master, "masterSpawnThreadReply should only be called on master.");
   LOG_PRINT("(4) masterSpawnThreadReply with req: { fun: %p, arg: %p, req: (%d, %d), req thread idx: %i, "
             "dest: (%d, %d), dest thread idx: %i dest thread id: %i }",
             req->func, req->arg, req->requester.tile_id, req->requester.core_type, req->requester_tidx,
             req->destination.tile_id, req->destination.core_type, req->destination_tidx, req->destination_tid);

   // Resume the requesting thread
   LOG_PRINT("masterSpawnThreadReply resuming thread: core(%i, %i) tidx(%i)",
             req->requester.tile_id, req->requester.core_type, req->requester_tidx);
   resumeThread(req->requester);

   SInt32 msg[] = { req->destination.tile_id, req->destination.core_type, req->destination_tidx, req->destination_tid };

   Core *core = m_tile_manager->getCurrentCore();
   core->getTile()->getNetwork()->netSend(req->requester, 
         MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE,
         msg,
         sizeof(req->destination.tile_id)+sizeof(req->destination.core_type)+sizeof(req->destination_tidx)+sizeof(req->destination_tid));
}

void ThreadManager::joinThread(thread_id_t join_thread_id)
{
   // Send the message to the master process; will get reply when thread is finished
   Core* core = m_tile_manager->getCurrentCore();
   thread_id_t thread_idx = m_tile_manager->getCurrentThreadIndex();

   ThreadJoinRequest msg = { MCP_MESSAGE_THREAD_JOIN_REQUEST,
                             core->getId(), thread_idx,
                             join_thread_id
                           };

   Network *net = core->getTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &msg,
                sizeof(msg));

   // Set the CoreState to 'STALLED'
   core->setState(Core::STALLED);

   // Wait for reply
   NetPacket pkt = net->netRecvType(MCP_THREAD_JOIN_REPLY, core->getId());

   // Set the CoreState to 'WAKING_UP'
   core->setState(Core::WAKING_UP);
}

void ThreadManager::masterJoinThread(ThreadJoinRequest *req, Time time)
{
   LOG_ASSERT_ERROR(m_master, "masterJoinThread should only be called on master.");
   LOG_PRINT("masterJoinThread called on thread %i", req->receiver_tid);

   thread_id_t join_thread_id = req->receiver_tid;
   LOG_ASSERT_ERROR(join_thread_id < (int) m_tid_to_core_map.size(), "A thread with ID(%i) has NOT been spawned", join_thread_id);

   core_id_t join_core_id = m_tid_to_core_map[join_thread_id].first;
   thread_id_t join_thread_idx =  m_tid_to_core_map[join_thread_id].second;

   // joinThread joins the current thread with the MAIN core's thread at tile_id. 
   LOG_PRINT("Joining thread [(%d), core ID(%d,%d), thread IDX(%d)], Requester[core ID(%d,%d), thread IDX(%d)]",
             join_thread_id, join_core_id.tile_id, join_core_id.core_type, join_thread_idx,
             req->sender.tile_id, req->sender.core_type, req->sender_tidx);

   //FIXME: fill in the proper time
   LOG_ASSERT_ERROR(m_thread_state[join_core_id.tile_id][join_thread_idx].waiter_core.tile_id == INVALID_TILE_ID,
                    "Multiple threads joining on tile %d", join_core_id.tile_id);

   LOG_ASSERT_ERROR((UInt32) join_core_id.tile_id < m_thread_state.size(), "Tile ID (%d) out of range", join_core_id.tile_id);
   
   m_thread_state[join_core_id.tile_id][join_thread_idx].waiter_core = req->sender;
   m_thread_state[join_core_id.tile_id][join_thread_idx].waiter_tid = req->sender_tidx;

   // Stall the 'pthread_join/CarbonJoinThread' caller
   stallThread(req->sender);

   // Tile not running, so the thread must have joined
   if (m_thread_state[join_core_id.tile_id][join_thread_idx].status == Core::IDLE)
   {
      LOG_PRINT("Not running, sending reply.");
      Time join_thread_completion_time = m_thread_state[join_core_id.tile_id][join_thread_idx].completion_time;
      wakeUpWaiter(join_core_id, join_thread_idx, join_thread_completion_time);
   }
}

bool ThreadManager::wakeUpWaiter(core_id_t core_id, thread_id_t thread_index, Time time)
{
   bool woken_up = false;
   LOG_ASSERT_ERROR(Tile::isMainCore(core_id), "Unrecognized core type to wake up");
   
   core_id_t waiter_core_id = m_thread_state[core_id.tile_id][thread_index].waiter_core;
   if (waiter_core_id.tile_id != INVALID_TILE_ID)
   {
      thread_id_t waiter_thread_id = m_thread_state[core_id.tile_id][thread_index].waiter_tid;
      LOG_PRINT("Waking up thread ID(%i) on core ID (%d,%d) at time: %llu ns",
                waiter_thread_id, waiter_core_id.tile_id, waiter_core_id.core_type, time.toNanosec());

      // Resume the 'pthread_join' caller
      LOG_PRINT("wakeUpWaiter resuming thread ID(%i) on core ID (%d,%d)", waiter_thread_id, waiter_core_id.tile_id, waiter_core_id.core_type);
      resumeThread(waiter_core_id, waiter_thread_id);

      // we have to manually send a packet because we are manufacturing a time-stamp
      Core *core = m_tile_manager->getCurrentCore();
      NetPacket pkt(time,
                    MCP_THREAD_JOIN_REPLY,
                    core->getId(),
                    waiter_core_id,
                    0,
                    NULL);
      core->getTile()->getNetwork()->netSend(pkt);

      m_thread_state[core_id.tile_id][thread_index].waiter_core = INVALID_CORE_ID;
      m_thread_state[core_id.tile_id][thread_index].waiter_tid = INVALID_THREAD_ID;

      woken_up = true;
   }
   LOG_PRINT("Exiting wakeUpWaiter");
   return woken_up;
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
         if (m_thread_spawners_terminated == (Config::getSingleton()->getProcessCount()-1))
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

thread_id_t ThreadManager::getNewThreadId(core_id_t core_id, thread_id_t thread_index)
{
   m_tid_counter_lock.acquire();

   m_tid_counter++;
   thread_id_t new_thread_id = m_tid_counter;

   if (m_tid_to_core_map.size() <= (UInt32) new_thread_id)
      m_tid_to_core_map.resize(2 * new_thread_id);

   m_tid_to_core_map[new_thread_id] = std::make_pair(core_id, thread_index);

   if (Tile::isMainCore(core_id))
      m_thread_state[core_id.tile_id][thread_index].thread_id = new_thread_id;
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return false;
   }

   LOG_PRINT("Generated tid(%i) for thread: core (%i,%i) tidx(%i)",
         new_thread_id, core_id.tile_id, core_id.core_type, thread_index);
   m_tid_counter_lock.release();
   return new_thread_id;
}

void ThreadManager::setOSTid(core_id_t core_id, thread_id_t thread_idx, pid_t os_tid)
{
   LOG_PRINT("ThreadManager::setOSTid called for %i on (%i, %i) with OS_tid %i",
         thread_idx, core_id.tile_id, core_id.core_type, os_tid);
   m_tid_map_lock.acquire();
   SInt32 req[] = { MCP_MESSAGE_THREAD_SET_OS_TID,
                   core_id.tile_id,
                   core_id.core_type,
                   thread_idx,
                   os_tid };

   Network *net = m_tile_manager->getCurrentCore()->getTile()->getNetwork();

   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   m_tid_map_lock.release();
}

void ThreadManager::masterSetOSTid(tile_id_t tile_id, thread_id_t thread_idx, pid_t os_tid)
{
   m_thread_state[tile_id][thread_idx].os_tid = os_tid;
}


void ThreadManager::queryThreadIndex(thread_id_t thread_id, core_id_t &core_id, thread_id_t &thread_idx, thread_id_t &next_tidx)
{
   LOG_PRINT("ThreadManager::queryThreadIndex called for tid %i", thread_id);
   m_tid_map_lock.acquire();
   SInt32 req[] = { MCP_MESSAGE_QUERY_THREAD_INDEX,
                  m_tile_manager->getCurrentCoreID().tile_id,
                  m_tile_manager->getCurrentCoreID().core_type,
                  thread_id};

   Network *net = m_tile_manager->getCurrentCore()->getTile()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreId(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   Core *core = m_tile_manager->getCurrentCore();
   NetPacket pkt = net->netRecvType(MCP_THREAD_QUERY_INDEX_REPLY_FROM_MASTER_TYPE, core->getId());
   LOG_ASSERT_ERROR(pkt.length == sizeof(ThreadIndexRequest), "Unexpected reply size.");

   ThreadIndexRequest * reply = (ThreadIndexRequest*) ((Byte*)pkt.data);
   LOG_ASSERT_ERROR(reply->thread_id == m_tile_manager->getCurrentThreadID(), "Received incorrect queryThreadIndex reply!");

   core_id = reply->core_id;
   thread_idx = reply->thread_idx;
   next_tidx = reply->next_tidx;
   m_tid_map_lock.release();
}

void ThreadManager::masterQueryThreadIndex(tile_id_t req_tile_id, UInt32 req_core_type, thread_id_t thread_id)
{
   LOG_PRINT("ThreadManager::masterQueryThreadIndex called for tid %i", thread_id);
   core_id_t requester = (core_id_t) {req_tile_id, req_core_type};
   core_id_t core_id = INVALID_CORE_ID;
   thread_id_t thread_idx = INVALID_THREAD_ID;

   lookupThreadIndex(thread_id, core_id, thread_idx);
   thread_id_t next_tidx = m_thread_scheduler->getNextThreadIdx(core_id);

   ThreadIndexRequest reply =   {   MCP_MESSAGE_QUERY_THREAD_INDEX,
                                    requester,
                                    thread_id,
                                    core_id,
                                    thread_idx,
                                    next_tidx
                                 };

   Core *core = m_tile_manager->getCurrentCore();
   core->getTile()->getNetwork()->netSend(requester, 
                                          MCP_THREAD_QUERY_INDEX_REPLY_FROM_MASTER_TYPE,
                                          &reply,
                                          sizeof(reply));
}

// Returns the thread INDEX and the core_id (by reference).
void ThreadManager::lookupThreadIndex(thread_id_t thread_id, core_id_t &core_id, thread_id_t &thread_idx)
{
   LOG_ASSERT_ERROR(thread_id <= m_tid_counter, "A thread with this tid %i has not been spawned before, the highest is %i!", thread_id, m_tid_counter);
   m_tid_counter_lock.acquire();

   core_id = m_tid_to_core_map[thread_id].first;
   thread_idx = m_tid_to_core_map[thread_id].second;

   m_tid_counter_lock.release();
}

// Returns the thread INDEX and the core_id (by reference).
void ThreadManager::setThreadIndex(thread_id_t thread_id, core_id_t core_id, thread_id_t thread_idx)
{
   LOG_ASSERT_ERROR(m_master, "setThreadIdx() must only be called on master");
   LOG_ASSERT_ERROR(thread_id < (int) m_tid_to_core_map.size(), "A thread with this tid has not been spawned before!");
   m_tid_counter_lock.acquire();

   m_tid_to_core_map[thread_id].first = core_id;
   m_tid_to_core_map[thread_id].second = thread_idx;  

   m_tid_counter_lock.release();
}

UInt32 ThreadManager::getNumScheduledThreads(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "getNumScheduledThreads() must only be called on master");

   UInt32 num_idle_threads = 0;
   Config * config = Config::getSingleton();

   // Find a free thread.
   if (Tile::isMainCore(core_id))
   {
      for (SInt32 j = 0; j < (SInt32) config->getMaxThreadsPerCore(); j++)
      {
         if (m_thread_state[core_id.tile_id][j].status == Core::IDLE)
         {
            num_idle_threads++;
         }
      }
   }
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
   }

   return config->getMaxThreadsPerCore() - num_idle_threads;
}

thread_id_t ThreadManager::getIdleThread(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "getIdleThread() must only be called on master");

   thread_id_t thread_idx = INVALID_THREAD_ID;
   Config* config = Config::getSingleton();

   // Find a free thread.
   if (Tile::isMainCore(core_id))
   {
      for (SInt32 j = 0; j < (SInt32) config->getMaxThreadsPerCore(); j++)
      {
         if (m_thread_state[core_id.tile_id][j].status == Core::IDLE)
         {
            thread_idx = j;
            break;
         }
      }
   }
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
   }

   return thread_idx;
}

// Stalls the current running thread.
void ThreadManager::stallThread(core_id_t core_id)
{
   thread_id_t thread_index = isCoreRunning(core_id);

   if(thread_index == INVALID_THREAD_ID)
   {
      LOG_ASSERT_ERROR(false, "Can't stall Core{%i, %i} because it has no running threads!", core_id.tile_id, core_id.core_type);
   }
   else
      stallThread(core_id, thread_index);
}

void ThreadManager::stallThread(core_id_t core_id, thread_id_t thread_index)
{
   LOG_ASSERT_ERROR(m_master, "stallThread() must only be called on master");
   LOG_PRINT("Core(%i, %i) tidx %i -> STALLED", core_id.tile_id, core_id.core_type, thread_index);

   if (Tile::isMainCore(core_id))
      stallThread(core_id.tile_id, thread_index);
   else
      LOG_PRINT_ERROR("Invalid core type!");
}

void ThreadManager::stallThread(tile_id_t tile_id, thread_id_t thread_index)
{
   m_thread_state[tile_id][thread_index].status = Core::STALLED;
   m_last_stalled_thread[tile_id] = thread_index;
   
   if (Sim()->getMCP()->getClockSkewManagementServer())
      Sim()->getMCP()->getClockSkewManagementServer()->signal();
}

void ThreadManager::resumeThread(core_id_t core_id)
{
   thread_id_t thread_index;
   if (Tile::isMainCore(core_id))
   {
      thread_index = m_last_stalled_thread[core_id.tile_id];
      assert(thread_index != INVALID_THREAD_ID);

      resumeThread(core_id.tile_id, thread_index);
   }
   else
      LOG_ASSERT_ERROR(false, "Invalid core type!");
}
void ThreadManager::resumeThread(core_id_t core_id, thread_id_t thread_index)
{
   LOG_PRINT("Core(%i, %i) -> RUNNING", core_id.tile_id, core_id.core_type);
   LOG_ASSERT_ERROR(m_master, "resumeThread() must only be called on master");

   if (Tile::isMainCore(core_id))
      resumeThread(core_id.tile_id, thread_index);
   else
      LOG_ASSERT_ERROR(false, "Invalid core type!");
}

void ThreadManager::resumeThread(tile_id_t tile_id, thread_id_t thread_index)
{
   m_thread_state[tile_id][thread_index].status = Core::RUNNING;
}

bool ThreadManager::isThreadRunning(core_id_t core_id, thread_id_t thread_index)
{
   LOG_ASSERT_ERROR(m_master, "isThreadRunning() must only be called on master");

   if (core_id.core_type == MAIN_CORE_TYPE)
      return isThreadRunning(core_id.tile_id, thread_index);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return 0;
   }
}

bool ThreadManager::isThreadRunning(tile_id_t tile_id, thread_id_t thread_index)
{
   return (m_thread_state[tile_id][thread_index].status == Core::RUNNING);
}

bool ThreadManager::isThreadInitializing(core_id_t core_id, thread_id_t thread_index)
{
   LOG_ASSERT_ERROR(m_master, "isThreadInitializing() must only be called on master");

   if (Tile::isMainCore(core_id))
      return isThreadInitializing(core_id.tile_id, thread_index);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return false;
   }

}

bool ThreadManager::isThreadInitializing(tile_id_t tile_id, thread_id_t thread_index)
{
   return (m_thread_state[tile_id][thread_index].status == Core::INITIALIZING);
}

bool ThreadManager::isCoreInitializing(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "isCoreInitializing() must only be called on master");

   if (Tile::isMainCore(core_id))
      return isCoreInitializing(core_id.tile_id);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return false;
   }

}

bool ThreadManager::isCoreInitializing(tile_id_t tile_id)
{
   bool is_core_initializing = false;
   for (SInt32 j = 0; j < (SInt32) m_thread_state[tile_id].size(); j++)
   {
      if (isThreadInitializing(tile_id, j))
      {
         is_core_initializing = true;
         break;
      }
   }

   return is_core_initializing;
}

bool ThreadManager::isThreadStalled(core_id_t core_id, thread_id_t thread_index)
{
   LOG_ASSERT_ERROR(m_master, "isThreadStalled() must only be called on master");

   if (Tile::isMainCore(core_id))
      return isThreadStalled(core_id.tile_id, thread_index);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return false;
   }

}

bool ThreadManager::isThreadStalled(tile_id_t tile_id, thread_id_t thread_index)
{
   return (m_thread_state[tile_id][thread_index].status == Core::STALLED);
}

bool ThreadManager::isCoreStalled(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "isCoreStalled() must only be called on master");

   if (Tile::isMainCore(core_id))
      return isCoreStalled(core_id.tile_id);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return false;
   }

}

bool ThreadManager::isCoreStalled(tile_id_t tile_id)
{
   bool is_core_stalled = false;
   for (SInt32 j = 0; j < (SInt32) m_thread_state[tile_id].size(); j++)
   {
      if (isThreadStalled(tile_id, j))
      {
         is_core_stalled = true;
         break;
      }
   }

   return is_core_stalled;
}



bool ThreadManager::areAllCoresRunning()
{
   LOG_ASSERT_ERROR(m_master, "areAllCoresRunning() should only be called on master.");

   // Check if all the cores are running
   bool is_all_running = true;
   thread_id_t thread_index = INVALID_THREAD_ID;
   for (SInt32 i = 0; i < (SInt32) m_thread_state.size(); i++)
   {
      thread_index = isCoreRunning(Tile::getMainCoreId(i));

      if (thread_index == INVALID_THREAD_ID)
      {
         is_all_running = false;
         break;
      }
   }

   return is_all_running;
}

// Returns INVALID_THREAD_ID if no threads are running
thread_id_t ThreadManager::isCoreRunning(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "isCoreRunning() must only be called on master");

   if (core_id.core_type == MAIN_CORE_TYPE)
      return isCoreRunning(core_id.tile_id);
   else
   {
      LOG_ASSERT_ERROR(false, "Invalid core type!");
      return INVALID_THREAD_ID;
   }
}

thread_id_t ThreadManager::isCoreRunning(tile_id_t tile_id)
{
   // Check if all the cores are running
   thread_id_t thread_index = INVALID_THREAD_ID;
   for (SInt32 j = 0; j < (SInt32) m_thread_state[tile_id].size(); j++)
   {
      if (isThreadRunning(tile_id, j))
      {
         if(thread_index == INVALID_THREAD_ID)
            thread_index = j;
         else
            LOG_PRINT_ERROR("Two threads on main core of tile %i are running simultaneously!", tile_id);
      }
   }

   return thread_index;
}

int ThreadManager::setThreadAffinity(pid_t os_tid, cpu_set_t* set)
{
   thread_id_t thread_index = INVALID_THREAD_ID;
   tile_id_t tile_id = INVALID_TILE_ID;
   int res = -1;

   for (SInt32 i = 0; i < (SInt32) m_thread_state.size(); i++)
   {
      for (SInt32 j = 0; j < (SInt32) m_thread_state[i].size(); j++)
      {
         if (m_thread_state[i][j].os_tid == os_tid)
         {
            if(thread_index == INVALID_THREAD_ID)
            {
               tile_id = i;
               thread_index = j;
            }
            else
               LOG_PRINT_ERROR("Two threads %i on %i and %i on %i have the same os_tid %i!", thread_index, tile_id, j, i, os_tid);
         }
      }
   }

   if (thread_index != INVALID_THREAD_ID)
   {
      res = 0;
      setThreadAffinity(tile_id, thread_index, set);
   }

   return res;
}


void ThreadManager::setThreadAffinity(tile_id_t tile_id, thread_id_t tidx, cpu_set_t* set)
{
   CPU_ZERO_S(CPU_ALLOC_SIZE(Config::getSingleton()->getTotalTiles()), m_thread_state[tile_id][tidx].cpu_set);
   CPU_OR_S(CPU_ALLOC_SIZE(Config::getSingleton()->getTotalTiles()), m_thread_state[tile_id][tidx].cpu_set, set, set);
}

int ThreadManager::getThreadAffinity(pid_t os_tid, cpu_set_t* set)
{
   thread_id_t thread_index = INVALID_THREAD_ID;
   tile_id_t tile_id = INVALID_TILE_ID;
   int res = -1;

   for (SInt32 i = 0; i < (SInt32) m_thread_state.size(); i++)
   {
      for (SInt32 j = 0; j < (SInt32) m_thread_state[i].size(); j++)
      {
         if (m_thread_state[i][j].os_tid == os_tid)
         {
            if(thread_index == INVALID_THREAD_ID)
            {
               tile_id = i;
               thread_index = j;
            }
            else
               LOG_PRINT_ERROR("Two threads %i on %i and %i on %i have the same os_tid %i!", thread_index, tile_id, j, i, os_tid);
         }
      }
   }

   if (thread_index != INVALID_THREAD_ID)
   {
      res = 0;
      getThreadAffinity(tile_id, thread_index, set);
   }

   return res;
}

void ThreadManager::getThreadAffinity(tile_id_t tile_id, thread_id_t tidx, cpu_set_t* set)
{
   CPU_ZERO_S(CPU_ALLOC_SIZE(Config::getSingleton()->getTotalTiles()), set);
   CPU_OR_S(CPU_ALLOC_SIZE(Config::getSingleton()->getTotalTiles()), set, m_thread_state[tile_id][tidx].cpu_set, set);
}


void ThreadManager::setThreadState(tile_id_t tile_id, thread_id_t tidx, ThreadManager::ThreadState state)
{
   m_thread_state[tile_id][tidx].status = state.status;
   m_thread_state[tile_id][tidx].waiter_core.tile_id = state.waiter_core.tile_id;
   m_thread_state[tile_id][tidx].waiter_core.core_type = state.waiter_core.core_type;
   m_thread_state[tile_id][tidx].waiter_tid = state.waiter_tid;
   m_thread_state[tile_id][tidx].thread_id = state.thread_id;
   m_thread_state[tile_id][tidx].cpu_set = state.cpu_set;
}

