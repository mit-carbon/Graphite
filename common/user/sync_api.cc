#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
#include "sync_client.h"
#include "config_file.hpp"
#include "carbon_user.h"
#include "thread_support_private.h"

void CarbonMutexInit(carbon_mutex_t *mux)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->mutexInit(mux);
}

void CarbonMutexLock(carbon_mutex_t *mux)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->mutexLock(mux);
}

void CarbonMutexUnlock(carbon_mutex_t *mux)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->mutexUnlock(mux);
}

void CarbonCondInit(carbon_cond_t *cond)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->condInit(cond);
}

void CarbonCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->condWait(cond, mux);
}

void CarbonCondSignal(carbon_cond_t *cond)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->condSignal(cond);
}

void CarbonCondBroadcast(carbon_cond_t *cond)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->condBroadcast(cond);
}

void CarbonBarrierInit(carbon_barrier_t *barrier, UInt32 count)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->barrierInit(barrier, count);
}

void CarbonBarrierWait(carbon_barrier_t *barrier)
{
   Tile *core = Sim()->getTileManager()->getCurrentTile();
   core->getSyncClient()->barrierWait(barrier);
}
