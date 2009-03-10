#include "user_space_wrappers.h"
#include "core.h"
#include "sync_client.h"
#include "core_manager.h"
#include "log.h"
#include "simulator.h"
#include "thread_manager.h"
#include "thread_support.h"

CAPI_return_t SimGetCoreID(int *core_id)
{
   *core_id = Sim()->getCoreManager()->getCurrentCoreID();
   return 0;
}

int SimGetProcessCount()
{
    return Config::getSingleton()->getProcessCount();
}

int SimGetProcessId()
{
    return Config::getSingleton()->getCurrentProcessNum();
}

void SimInitializeCommId(int comm_id)
{
   Sim()->getCoreManager()->initializeCommId(comm_id);
}

int SimSpawnThread(thread_func_t func, void *arg)
{
   return Sim()->getThreadManager()->spawnThread(func, arg);
}

void SimJoinThread(int tid)
{
   Sim()->getThreadManager()->joinThread(tid);
}

int SimPthreadCreate(pthread_t *tid, int *attr, thread_func_t func, void *arg)
{
   LOG_ASSERT_WARNING(attr == NULL, "Attributes ignored in pthread_create.");
   LOG_ASSERT_ERROR(tid != NULL, "Null pointer passed to pthread_create.");

   *tid = SimSpawnThread(func, arg);
   return *tid >= 0 ? 0 : 1;
}

int SimPthreadJoin(pthread_t tid, void **pparg)
{
   LOG_ASSERT_WARNING(pparg == NULL, "Did not expect pparg non-NULL. It is ignored.");
   SimJoinThread(tid);
   return 0;
}

void SimMutexInit(carbon_mutex_t *mux)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->mutexInit(mux);
}

void SimMutexLock(carbon_mutex_t *mux)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->mutexLock(mux);
}

void SimMutexUnlock(carbon_mutex_t *mux)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->mutexUnlock(mux);
}

void SimCondInit(carbon_cond_t *cond)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->condInit(cond);
}

void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->condWait(cond, mux);
}

void SimCondSignal(carbon_cond_t *cond)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->condSignal(cond);
}

void SimCondBroadcast(carbon_cond_t *cond)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->condBroadcast(cond);
}

void SimBarrierInit(carbon_barrier_t *barrier, UInt32 count)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->barrierInit(barrier, count);
}

void SimBarrierWait(carbon_barrier_t *barrier)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->barrierWait(barrier);
}

void SimThreadStart(ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->onThreadStart(req);
}

void SimThreadExit()
{
   Sim()->getThreadManager()->onThreadExit();
}

void SimGetThreadToSpawn(ThreadSpawnRequest **req)
{
   Sim()->getThreadManager()->getThreadToSpawn(req);
}

CAPI_return_t SimSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d", sender, receiver);

   core_id_t sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   core_id_t receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   if(sending_core == INVALID_CORE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_core == INVALID_CORE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreSendW(sending_core, receiving_core, buffer, size) : CAPI_SenderNotInitialized;
}

CAPI_return_t SimRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d", sender, receiver);

   core_id_t sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   core_id_t receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   if(sending_core == INVALID_CORE_ID)
       return CAPI_SenderNotInitialized;
   if(receiving_core == INVALID_CORE_ID)
       return CAPI_ReceiverNotInitialized;

   return core ? core->coreRecvW(sending_core, receiving_core, buffer, size) : CAPI_SenderNotInitialized;
}
