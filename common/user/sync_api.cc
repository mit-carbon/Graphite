#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "tile_manager.h"
#include "core.h"
#include "sync_client.h"
#include "config_file.hpp"
#include "carbon_user.h"
#include "thread_support_private.h"

void CarbonMutexInit(carbon_mutex_t *mux)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->mutexInit(mux);
}

void CarbonMutexLock(carbon_mutex_t *mux)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->mutexLock(mux);
}

void CarbonMutexUnlock(carbon_mutex_t *mux)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->mutexUnlock(mux);
}

void CarbonCondInit(carbon_cond_t *cond)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->condInit(cond);
}

void CarbonCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->condWait(cond, mux);
}

void CarbonCondSignal(carbon_cond_t *cond)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->condSignal(cond);
}

void CarbonCondBroadcast(carbon_cond_t *cond)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->condBroadcast(cond);
}

void CarbonBarrierInit(carbon_barrier_t *barrier, UInt32 count)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->barrierInit(barrier, count);
}

void CarbonBarrierWait(carbon_barrier_t *barrier)
{
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->getSyncClient()->barrierWait(barrier);
}
