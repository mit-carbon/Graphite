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
      m_thread_state[config->getMCPCoreNum()].running = true;
      LOG_PRINT("%d", config->getMCPCoreNum());
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

      for (UInt32 i = 0; i < m_thread_state.size(); i++)
         LOG_ASSERT_WARNING(!m_thread_state[i].running, "Thread %d still active when ThreadManager destructs!", i);
   }
}

void ThreadManager::onThreadStart(ThreadSpawnRequest *req)
{
   m_core_manager->initializeThread(req->core_id);

   // send ack to master
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE;
   globalNode->globalSend(0, req, sizeof(*req));

   free(req);
}

void ThreadManager::onThreadExit()
{
   // send message to master process to update thread state
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   SInt32 msg[4] = { LCP_MESSAGE_THREAD_EXIT, m_core_manager->getCurrentCoreID(), 0, 0 };
   LOG_PRINT("onThreadExit msg: { %d, %d, %u, %u }", msg[0], msg[1], msg[2], msg[3]);
   globalNode->globalSend(0, msg, sizeof(msg));

   m_core_manager->terminateThread();
}

void ThreadManager::masterOnThreadExit(core_id_t core_id, UInt64 time)
{
   LOG_ASSERT_ERROR(m_master, "masterOnThreadExit should only be called on master.");
   LOG_PRINT("masterOnThreadExit : %d %llu", core_id, time);
   LOG_ASSERT_ERROR((UInt32)core_id < m_thread_state.size(), "Core id out of range: %d", core_id);
   assert(m_thread_state[core_id].running);
   m_thread_state[core_id].running = false;

   wakeUpWaiter(core_id);
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

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   Core *core = m_core_manager->getCurrentCore();

   ThreadSpawnRequest req = { LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER, 
                              func, arg, core->getId(), INVALID_CORE_ID };

   globalNode->globalSend(0, &req, sizeof(req));

   NetPacket pkt = core->getNetwork()->netRecvType(LCP_SPAWN_THREAD_REPLY_FROM_MASTER_TYPE);
   LOG_ASSERT_ERROR(pkt.length == sizeof(SInt32), "Unexpected reply size.");

   core_id_t core_id = *((SInt32*)pkt.data);
   LOG_PRINT("Thread spawned on core: %d", core_id);

   return *((SInt32*)pkt.data);
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
   else //if it's local then just add it to the local list
   {
      ThreadSpawnRequest *req_cpy = (ThreadSpawnRequest *)malloc(sizeof(ThreadSpawnRequest));
      *req_cpy = *req;

      m_thread_spawn_lock.acquire();
      m_thread_spawn_list.push(req_cpy);
      m_thread_spawn_lock.release();
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
   ThreadSpawnRequest *req_cpy = (ThreadSpawnRequest *)malloc(sizeof(ThreadSpawnRequest));
   *req_cpy = *req;

   m_thread_spawn_lock.acquire();
   m_thread_spawn_list.push(req_cpy);
   m_thread_spawn_lock.release();
   m_thread_spawn_sem.signal();
}

void ThreadManager::getThreadToSpawn(ThreadSpawnRequest **req)
{
   // step 4
   LOG_PRINT("(4a) getThreadToSpawn called by user.");

   // Wait for a request to arrive
   m_thread_spawn_sem.wait();

   // Grab the request and set the argument
   m_thread_spawn_lock.acquire();
   *req = m_thread_spawn_list.front();
   m_thread_spawn_list.pop();
   m_thread_spawn_lock.release();

   LOG_PRINT("(4b) getThreadToSpawn giving thread %p arg: %p to user.", (*req)->func, (*req)->arg);
}


void ThreadManager::masterSpawnThreadReply(ThreadSpawnRequest *req)
{
   // step 6
   LOG_ASSERT_ERROR(m_master, "masterSpawnThreadReply should only be called on master.");
   LOG_PRINT("(6) masterSpawnThreadReply with req: { fun: %p, arg: %p, req: %d, core: %d }", req->func, req->arg, req->requester, req->core_id);

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   NetPacket pkt(0 /*time*/, LCP_SPAWN_THREAD_REPLY_FROM_MASTER_TYPE, 
                 0 /*sender*/, req->requester, sizeof(req->core_id), &req->core_id);
   Byte *buffer = pkt.makeBuffer();
   globalNode->send(req->requester, buffer, pkt.bufferSize());
   delete [] buffer;
}

void ThreadManager::joinThread(core_id_t core_id)
{
   LOG_PRINT("Joining on core: %d", core_id);

   // Send the message to the master process; will get reply when thread is finished
   ThreadJoinRequest msg;
   msg.msg_type = LCP_MESSAGE_THREAD_JOIN_REQUEST;
   msg.core_id = core_id;
   msg.sender = m_core_manager->getCurrentCoreID();

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   globalNode->globalSend(0, &msg, sizeof(msg));

   // Wait for reply
   Core *core = m_core_manager->getCurrentCore();
   NetPacket pkt = core->getNetwork()->netRecvType(LCP_JOIN_THREAD_REPLY);
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
   if (m_thread_state[core_id].waiter != -1)
   {
      LOG_PRINT("Waking up core: %d", m_thread_state[core_id].waiter);

      Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
      NetPacket pkt(0 /*time*/, LCP_JOIN_THREAD_REPLY,
                    0 /*sender*/, m_thread_state[core_id].waiter, 0, NULL);
      Byte *buffer = pkt.makeBuffer();
      globalNode->send(m_thread_state[core_id].waiter, buffer, pkt.bufferSize());
      delete [] buffer;

      m_thread_state[core_id].waiter = -1;
   }
   LOG_PRINT("Exiting wakeUpWaiter");
}

