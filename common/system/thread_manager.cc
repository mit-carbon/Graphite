#include <sys/syscall.h>
#include "thread_manager.h"
#include "core_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "simulator.h"
#include "network.h"
#include "message_types.h"
#include "core.h"
#include "thread.h"
#include "packetize.h"

ThreadManager::ThreadManager(CoreManager *core_manager)
   : m_thread_spawn_sem(0)
   , m_thread_spawn_lock()
   , m_core_manager(core_manager)
{
   Config *config = Config::getSingleton();

   m_master = config->getCurrentProcessNum() == 0;

   if (m_master)
   {
      m_thread_state.resize(config->getTotalCores());
      m_thread_state[0].running = true;

      // Reserve core-id's 1 to (num_processes) for thread-spawners
      UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalCores() - Sim()->getConfig()->getProcessCount() - 1;
      UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalCores() - 2;
      for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
      {
         m_thread_state[i].running = true;
      }

      m_thread_state[config->getMCPCoreNum()].running = true;
      
      LOG_ASSERT_ERROR(config->getMCPCoreNum() < (SInt32)m_thread_state.size(),
                       "MCP core num out of range (!?)");
   }
}

ThreadManager::~ThreadManager()
{
   if (m_master)
   {
      m_thread_state[0].running = false;
      m_thread_state[Config::getSingleton()->getMCPCoreNum()].running = false;
      LOG_ASSERT_ERROR(Config::getSingleton()->getMCPCoreNum() < (SInt32)m_thread_state.size(), "MCP core num out of range (!?)");

      // Reserve core-id's 1 to (num_processes) for thread-spawners
      UInt32 first_thread_spawner_id = Sim()->getConfig()->getTotalCores() - Sim()->getConfig()->getProcessCount() - 1;
      UInt32 last_thread_spawner_id = Sim()->getConfig()->getTotalCores() - 2;
      for (UInt32 i = first_thread_spawner_id; i <= last_thread_spawner_id; i++)
      {
         m_thread_state[i].running = false;
      }

      for (UInt32 i = 0; i < m_thread_state.size(); i++)
         LOG_ASSERT_WARNING(!m_thread_state[i].running, "Thread %d still active when ThreadManager destructs!", i);
   }
}

void ThreadManager::onThreadStart(ThreadSpawnRequest *req)
{
   m_core_manager->initializeThread(req->core_id);

   if (req->core_id == Sim()->getConfig()->getCurrentThreadSpawnerCoreNum())
      return;
 
   // send ack to master
   LOG_PRINT("(5) onThreadStart -- send ack to master; req : { %p, %p, %i, %i }",
             req->func, req->arg, req->requester, req->core_id);

   req->msg_type = MCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE;
   Network *net = m_core_manager->getCurrentCore()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreNum(),
                MCP_REQUEST_TYPE,
                req,
                sizeof(*req));
}

void ThreadManager::onThreadExit()
{
   if (m_core_manager->getCurrentCoreID() == -1)
      return;
   
   // send message to master process to update thread state
   SInt32 msg[] = { MCP_MESSAGE_THREAD_EXIT, m_core_manager->getCurrentCoreID() };

   LOG_PRINT("onThreadExit -- send message to master ThreadManager");
   
   Network *net = m_core_manager->getCurrentCore()->getNetwork();

   // terminate thread locally so we are ready for new thread requests
   // on that core
   m_core_manager->terminateThread();

   // update global thread state
   net->netSend(Config::getSingleton()->getMCPCoreNum(),
                MCP_REQUEST_TYPE,
                msg,
                sizeof(SInt32)*2);
}

void ThreadManager::dequeueThreadSpawnReq (ThreadSpawnRequest *req)
{
   ThreadSpawnRequest *thread_req = m_thread_spawn_list.front();

   *req = *thread_req;

   m_thread_spawn_list.pop();
   m_thread_spawn_lock.release();

   delete thread_req;

   LOG_PRINT("Dequeued req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);
}

void ThreadManager::masterOnThreadExit(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_master, "masterOnThreadExit should only be called on master.");
   LOG_PRINT("masterOnThreadExit : %d", core_id);
   LOG_ASSERT_ERROR((UInt32)core_id < m_thread_state.size(), "Core id out of range: %d", core_id);
   assert(m_thread_state[core_id].running);
   m_thread_state[core_id].running = false;

   wakeUpWaiter(core_id);

   Config *config = Config::getSingleton();
   for (UInt32 i = 0; i < config->getProcessCount(); i++)
   {
      if (core_id == config->getThreadSpawnerCoreNum (i))
      {
         Network *net = m_core_manager->getCurrentCore()->getNetwork();

         UnstructuredBuffer buf;

         int req_type = MCP_MESSAGE_QUIT_THREAD_SPAWNER_ACK;
         buf << req_type << i;

         net->netSend (config->getMainThreadCoreNum(),
               MCP_RESPONSE_TYPE,
               buf.getBuffer(),
               buf.size());
      }
   }
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
   // step 1
   LOG_PRINT("(1) spawnThread with func: %p and arg: %p", func, arg);

   Core *core = m_core_manager->getCurrentCore();
   Network *net = core->getNetwork();

   ThreadSpawnRequest req = { MCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER, 
                              func, arg, core->getId(), INVALID_CORE_ID };

   net->netSend(Config::getSingleton()->getMCPCoreNum(),
                MCP_REQUEST_TYPE,
                &req,
                sizeof(req));

   NetPacket pkt = net->netRecvType(MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE);
   LOG_ASSERT_ERROR(pkt.length == sizeof(SInt32), "Unexpected reply size.");

   core_id_t core_id = *((core_id_t*)pkt.data);
   LOG_PRINT("Thread spawned on core: %d", core_id);

   return core_id;
}

void ThreadManager::masterSpawnThread(ThreadSpawnRequest *req)
{
   // step 2
   LOG_ASSERT_ERROR(m_master, "masterSpawnThread should only be called on master.");
   LOG_PRINT("(2) masterSpawnThread with req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);
 
   // find core to use
   // FIXME: Load balancing?
   for (SInt32 i = 0; i < (SInt32)m_thread_state.size(); i++)
   {
      if (!m_thread_state[i].running)
      {
         req->core_id = i;
         break;
      }
   }

   LOG_ASSERT_ERROR(req->core_id != INVALID_CORE_ID, "No cores available for spawnThread request.");

   // spawn process on child
   SInt32 dest_proc = Config::getSingleton()->getProcessNumForCore(req->core_id);
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_MASTER;

   if(dest_proc != (SInt32)Config::getSingleton()->getCurrentProcessNum())
   {
      LOG_PRINT("Sending thread spawn request to proc: %d", dest_proc);
      globalNode->globalSend(dest_proc, req, sizeof(*req));
      LOG_PRINT("Sent thread spawn request to proc: %d", dest_proc);
   }
   else 
   {
      // if it's local then just add it to the local list.
      // this is a short-circuit hack to fix the normal mechanism not
      // working on the same process.
      ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest;
      *req_cpy = *req;

      // Insert the request in the thread request queue and
      // the thread request map
      insertThreadSpawnRequest (req_cpy);
      m_thread_spawn_sem.signal();

   }

   LOG_ASSERT_ERROR((UInt32)req->core_id < m_thread_state.size(), "Core id out of range: %d", req->core_id);
   m_thread_state[req->core_id].running = true;
   LOG_PRINT("Done with (2)");
}

void ThreadManager::slaveSpawnThread(ThreadSpawnRequest *req)
{
   // step 3
   LOG_PRINT("(3) slaveSpawnThread with req: { fun: %p, arg: %p, req: %d, core: %d }", req->func, req->arg, req->requester, req->core_id);

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


void ThreadManager::masterSpawnThreadReply(ThreadSpawnRequest *req)
{
   // step 6
   LOG_ASSERT_ERROR(m_master, "masterSpawnThreadReply should only be called on master.");
   LOG_PRINT("(6) masterSpawnThreadReply with req: { fun: %p, arg: %p, req: %d, core: %d }", req->func, req->arg, req->requester, req->core_id);

   Core *core = m_core_manager->getCurrentCore();
   core->getNetwork()->netSend(req->requester, 
                               MCP_THREAD_SPAWN_REPLY_FROM_MASTER_TYPE,
                               &req->core_id,
                               sizeof(req->core_id));
}

void ThreadManager::joinThread(core_id_t core_id)
{
   LOG_PRINT("Joining on core: %d", core_id);

   // Send the message to the master process; will get reply when thread is finished
   ThreadJoinRequest msg;
   msg.msg_type = MCP_MESSAGE_THREAD_JOIN_REQUEST;
   msg.core_id = core_id;
   msg.sender = m_core_manager->getCurrentCoreID();

   Network *net = m_core_manager->getCurrentCore()->getNetwork();
   net->netSend(Config::getSingleton()->getMCPCoreNum(),
                MCP_REQUEST_TYPE,
                &msg,
                sizeof(msg));

   // Wait for reply
   NetPacket pkt = net->netRecvType(MCP_THREAD_JOIN_REPLY);
   // delete [] (Byte*)pkt.data;   *don't* delete the data since this packet has none

   LOG_PRINT("Exiting join thread.");
}

void ThreadManager::masterJoinThread(ThreadJoinRequest *req)
{
   LOG_ASSERT_ERROR(m_master, "masterJoinThread should only be called on master.");
   LOG_PRINT("masterJoinThread called on core: %d", req->core_id);
   //FIXME: fill in the proper time

   LOG_ASSERT_ERROR(m_thread_state[req->core_id].waiter == -1,
                    "Multiple threads joining on thread: %d", req->core_id);
   m_thread_state[req->core_id].waiter = req->sender;

   // Core not running, so the thread must have joined
   LOG_ASSERT_ERROR((UInt32)req->core_id < m_thread_state.size(), "Core id out of range: %d", req->core_id);
   if(m_thread_state[req->core_id].running == false)
   {
      LOG_PRINT("Not running, sending reply.");
      wakeUpWaiter(req->core_id);
   }
}

void ThreadManager::wakeUpWaiter(core_id_t core_id)
{
   if (m_thread_state[core_id].waiter != INVALID_CORE_ID)
   {
      LOG_PRINT("Waking up core: %d", m_thread_state[core_id].waiter);

      Network *net = m_core_manager->getCurrentCore()->getNetwork();
      net->netSend(m_thread_state[core_id].waiter,
                   MCP_THREAD_JOIN_REPLY,
                   NULL,
                   0);

      m_thread_state[core_id].waiter = INVALID_CORE_ID;
   }
   LOG_PRINT("Exiting wakeUpWaiter");
}

void ThreadManager::insertThreadSpawnRequest (ThreadSpawnRequest *req)
{
   // Insert the request in the thread request queue
   m_thread_spawn_lock.acquire();
   m_thread_spawn_list.push(req);
   m_thread_spawn_lock.release();
}
  
void ThreadManager::terminateThreadSpawner ()
{
   LOG_PRINT ("In terminateThreadSpawner");

   Network *net = m_core_manager->getCurrentCore()->getNetwork();

   for (UInt32 pid = 0; pid < Config::getSingleton()->getProcessCount(); pid++)
   {
      UnstructuredBuffer buf;
      int send_pkt_type = MCP_MESSAGE_QUIT_THREAD_SPAWNER;
      buf << send_pkt_type << pid;

      net->netSend (Config::getSingleton()->getMCPCoreNum(),
            MCP_REQUEST_TYPE,
            buf.getBuffer(),
            buf.size());

      NetPacket pkt = net->netRecvType (MCP_RESPONSE_TYPE);
      LOG_ASSERT_ERROR (pkt.length == (sizeof (int) + sizeof (UInt32)), "Unexpected reply size");

      buf.clear();

      buf << make_pair (pkt.data, pkt.length);

      int recv_pkt_type;
      UInt32 process_id;

      buf >> recv_pkt_type >> process_id;

      LOG_ASSERT_ERROR (recv_pkt_type == MCP_MESSAGE_QUIT_THREAD_SPAWNER_ACK, "Received unexpected packet from MCP while waiting for thread spawner quit ack");
      LOG_ASSERT_ERROR (process_id == pid, "Thread spawner quit ack from unexpected source");

      LOG_PRINT ("Thread spanwer for process %d terminated", pid);
   }
}

void ThreadManager::masterTerminateThreadSpawner(UInt32 proc)
{
   LOG_ASSERT_ERROR (m_master, "masterTerminateThreadSpawner should only be called on master");
   LOG_PRINT ("masterTerminateThreadSpawner with thread spawner quit req for proc %d", proc);

   if (proc != Config::getSingleton()->getCurrentProcessNum())
   {
      Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

      int send_req = LCP_MESSAGE_QUIT_THREAD_SPAWNER;
      
      LOG_PRINT ("Sending thread spawner quit message to proc %d", proc);
      globalNode->globalSend (proc, &send_req, sizeof (send_req));
      LOG_PRINT ("Sent thread spawner quit message to proc %d", proc);
   }
   else
   {
      // FIXME: 
      // Do we still need this hack?
      
      // This is a short circuit hack if the thread spawner quit request 
      // is for the same process

      slaveTerminateThreadSpawner();
   }
}

void ThreadManager::slaveTerminateThreadSpawner ()
{
   LOG_PRINT ("slaveTerminateThreadSpawner on proc %d", Config::getSingleton()->getCurrentProcessNum());

   ThreadSpawnRequest *req = new ThreadSpawnRequest;

   req->msg_type = LCP_MESSAGE_QUIT_THREAD_SPAWNER;
   req->func = NULL;
   req->arg = NULL;
   req->requester = INVALID_CORE_ID;
   req->core_id = INVALID_CORE_ID;

   insertThreadSpawnRequest (req);
   m_thread_spawn_sem.signal();
}
