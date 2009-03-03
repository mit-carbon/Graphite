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

void SimInitializeThread()
{
   Sim()->getCoreManager()->initializeThread();
}

void SimTerminateThread()
{
   Sim()->getCoreManager()->terminateThread();
}

int SimGetProcessCount()
{
    return Config::getSingleton()->getProcessCount();
}

int SimGetProcessId()
{
   LOG_ASSERT_WARNING(attr == NULL, "Attributes ignored in pthread_create.");
   LOG_ASSERT_ERROR(tid != NULL, "Null pointer passed to pthread_create.");

   *tid = SimSpawnThread(func, arg);
   return *tid >= 0 ? 0 : 1;
}

void SimInitializeCommId(int comm_id)
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

void SimBarrierInit(carbon_barrier_t *barrier, UINT32 count)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->barrierInit(barrier, count);
}

void SimBarrierWait(carbon_barrier_t *barrier)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core) core->getSyncClient()->barrierWait(barrier);
}


CAPI_return_t SimSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimSendW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   UInt32 receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   return core ? core->coreSendW(sending_core, receiving_core, buffer, size) : -1;
}

CAPI_return_t SimRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   LOG_PRINT("SimRecvW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = Config::getSingleton()->getCoreFromCommId(sender);
   UInt32 receiving_core = Config::getSingleton()->getCoreFromCommId(receiver);

   return core ? core->coreRecvW(sending_core, receiving_core, buffer, size) : -1;
}

