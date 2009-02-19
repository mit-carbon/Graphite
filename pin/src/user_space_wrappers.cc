#include "user_space_wrappers.h"
#include "core.h"
#include "sync_client.h"
#include "core_manager.h"
#include "log.h"

CAPI_return_t SimGetCoreID(int *core_id)
{
   *core_id = g_core_manager->getCurrentCoreID();
   return 0;
}

void SimInitializeThread()
{
   g_core_manager->initializeThread();
}

void SimInitializeCommId(int comm_id)
{
   g_core_manager->initializeCommId(comm_id);
}

void SimMutexInit(carbon_mutex_t *mux)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->mutexInit(mux);
}

void SimMutexLock(carbon_mutex_t *mux)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->mutexLock(mux);
}

void SimMutexUnlock(carbon_mutex_t *mux)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->mutexUnlock(mux);
}

void SimCondInit(carbon_cond_t *cond)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->condInit(cond);
}

void SimCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->condWait(cond, mux);
}

void SimCondSignal(carbon_cond_t *cond)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->condSignal(cond);
}

void SimCondBroadcast(carbon_cond_t *cond)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->condBroadcast(cond);
}

void SimBarrierInit(carbon_barrier_t *barrier, UINT32 count)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->barrierInit(barrier, count);
}

void SimBarrierWait(carbon_barrier_t *barrier)
{
   Core *core = g_core_manager->getCurrentCore();
   if (core) core->getSyncClient()->barrierWait(barrier);
}


CAPI_return_t SimSendW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = g_core_manager->getCurrentCore();

   LOG_PRINT_EXPLICIT(-1, USERSPACEWRAPPERS, "SimSendW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = g_config->getCoreFromCommId(sender);
   UInt32 receiving_core = g_config->getCoreFromCommId(receiver);

   return core ? core->coreSendW(sending_core, receiving_core, buffer, size) : -1;
}

CAPI_return_t SimRecvW(CAPI_endpoint_t sender, CAPI_endpoint_t receiver,
                       char *buffer, int size)
{
   Core *core = g_core_manager->getCurrentCore();

   LOG_PRINT_EXPLICIT(-1, USERSPACEWRAPPERS, "SimRecvW - sender: %d, recv: %d", sender, receiver);

   UInt32 sending_core = g_config->getCoreFromCommId(sender);
   UInt32 receiving_core = g_config->getCoreFromCommId(receiver);

   return core ? core->coreRecvW(sending_core, receiving_core, buffer, size) : -1;
}

