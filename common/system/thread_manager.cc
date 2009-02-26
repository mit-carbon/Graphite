#include "thread_manager.h"
#include "core_manager.h"
#include "config.h"
#include "log.h"
#include "transport.h"
#include "simulator.h"
#include "network.h"
#include "message_types.h"
#include "core.h"

#define LOG_DEFAULT_RANK -1
#define LOG_DEFAULT_MODULE THREAD_MANAGER

ThreadManager::ThreadManager(CoreManager *core_manager)
   : m_core_manager(core_manager)
{
   Config *config = Config::getSingleton();

   m_master = config->getCurrentProcessNum() == 0;

   if (m_master)
   {
      m_thread_state.resize(config->getTotalCores());
      m_thread_state[0].running = true;
      m_thread_state[config->getMCPCoreNum()].running = true;
      LOG_PRINT("%d", config->getMCPCoreNum());
   }
}

ThreadManager::~ThreadManager()
{
   if (m_master)
   {
      m_thread_state[0].running = false;
      m_thread_state[Config::getSingleton()->getMCPCoreNum()].running = false;

      for (UInt32 i = 0; i < m_thread_state.size(); i++)
         LOG_ASSERT_WARNING(!m_thread_state[i].running, "*WARNING* Thread %d still active when ThreadManager destructs!", i);
   }
}

void ThreadManager::onThreadStart(SInt32 core_id)
{
   m_core_manager->initializeThread(core_id);
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

void ThreadManager::masterOnThreadExit(SInt32 core_id, UInt64 time)
{
   LOG_PRINT("masterOnThreadExit : %d %llu", core_id, time);
   assert(m_thread_state[core_id].running);
   m_thread_state[core_id].running = false;

   wakeUpWaiter(core_id);
}

/*
  Thread spawning occurs in the following steps:

  1. A message is sent from requestor to the master thread manager.
  2. The master thread manager finds the destination core and sends a message to its host process.
  3. The host process spawns the new thread and returns an ack to the master thread manager.
  4. The master thread manager replies to the requestor with the id of the dest core.
*/

SInt32 ThreadManager::spawnThread(void (*func)(void*), void *arg)
{
   LOG_PRINT("spawnThread with func: %p and arg: %p", func, arg);

   // step 1
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   Core *core = m_core_manager->getCurrentCore();

   ThreadSpawnRequest req = { LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_REQUESTER, 
                              func, arg, core->getId(), -1 };

   globalNode->globalSend(0, &req, sizeof(req));

   NetPacket pkt = core->getNetwork()->netRecvType(LCP_SPAWN_THREAD_REPLY_FROM_MASTER_TYPE);
   LOG_ASSERT_ERROR(pkt.length == sizeof(SInt32), "*ERROR* Unexpected reply size.");

   SInt32 core_id = *((SInt32*)pkt.data);
   LOG_PRINT("Thread spawned on core: %d", core_id);

   return *((SInt32*)pkt.data);
}

void ThreadManager::masterSpawnThread(ThreadSpawnRequest *req)
{
   LOG_PRINT("masterSpawnThread with req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);
   // step 2
 
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

   LOG_ASSERT_ERROR(req->core_id != -1, "*ERROR* No cores available for spawnThread request.");

   // spawn process on child
   SInt32 dest_proc = Config::getSingleton()->getProcessNumForCore(req->core_id);
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REQUEST_FROM_MASTER;

   globalNode->globalSend(dest_proc, req, sizeof(*req));
   m_thread_state[req->core_id].running = true;
}

void ThreadManager::slaveSpawnThread(ThreadSpawnRequest *req)
{
   LOG_PRINT("slaveSpawnThread with req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);
   // step 3

   ThreadSpawnRequest *req_cpy = new ThreadSpawnRequest(*req);

   Thread *t = Thread::create(spawnedThreadFunc, req_cpy);
   t->run();
}

void ThreadManager::masterSpawnThreadReply(ThreadSpawnRequest *req)
{
   LOG_PRINT("masterSpawnThreadReply with req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);
   // step 4

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   NetPacket pkt(0 /*time*/, LCP_SPAWN_THREAD_REPLY_FROM_MASTER_TYPE, 
                 0 /*sender*/, req->requester, sizeof(SInt32), &req->core_id);
   Byte *buffer = pkt.makeBuffer();
   globalNode->send(req->requester, buffer, pkt.bufferSize());
   delete [] buffer;
}

void ThreadManager::spawnedThreadFunc(void *vpreq)
{
   ThreadSpawnRequest *req = (ThreadSpawnRequest*) vpreq;
   LOG_PRINT("spawnedThreadFunc with req: { %p, %p, %d, %d }", req->func, req->arg, req->requester, req->core_id);

   Sim()->getThreadManager()->onThreadStart(req->core_id);

   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
   req->msg_type = LCP_MESSAGE_THREAD_SPAWN_REPLY_FROM_SLAVE;
   globalNode->globalSend(0, req, sizeof(*req));

   req->func(req->arg);

   Sim()->getThreadManager()->onThreadExit();

   delete req;
}

void ThreadManager::joinThread(SInt32 core_id)
{
   // Obtain the object that allows us to communicate to other processes
   Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();

   // create a condition variable to wait for the reply
   ThreadJoinRequest msg;
   msg.core_id = core_id;
   msg.sender = m_core_manager->getCurrentCoreID();

   // Send the message to the master process
   globalNode->globalSend(0, &msg, sizeof(msg));

   // Wait for reply
   Core *core = m_core_manager->getCurrentCore();
   NetPacket pkt = core->getNetwork()->netRecvType(LCP_JOIN_THREAD_REPLY);
}

void ThreadManager::masterJoinThread(ThreadJoinRequest *req)
{
   LOG_PRINT("masterJoinThread called.");
   //FIXME: fill in the proper time

   LOG_ASSERT_ERROR(m_thread_state[req->core_id].waiter == -1,
                    "*ERROR* Multiple threads joining on thread: %d", req->core_id);
   m_thread_state[req->core_id].waiter = req->sender;

   // Core not running, so the thread must have joined
   if(m_thread_state[req->core_id].running == false)
   {
      LOG_PRINT("Not running, sending reply.");
      wakeUpWaiter(req->core_id);
   }
}

void ThreadManager::wakeUpWaiter(SInt32 core_id)
{
   if (m_thread_state[core_id].waiter != -1)
   {
      Transport::Node *globalNode = Transport::getSingleton()->getGlobalNode();
      NetPacket pkt(0 /*time*/, LCP_JOIN_THREAD_REPLY,
                    0 /*sender*/, m_thread_state[core_id].waiter, 0, NULL);
      Byte *buffer = pkt.makeBuffer();
      globalNode->send(m_thread_state[core_id].waiter, buffer, pkt.bufferSize());
      delete [] buffer;

      m_thread_state[core_id].waiter = -1;
   }
}
