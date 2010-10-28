#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "tile.h"
#include "sync_client.h"
#include "config_file.hpp"
#include "carbon_user.h"
#include "thread_support_private.h"

void CarbonMutexInit(carbon_mutex_t *mux)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->mutexInit(mux);
}

void CarbonMutexLock(carbon_mutex_t *mux)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->mutexLock(mux);
}

void CarbonMutexUnlock(carbon_mutex_t *mux)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->mutexUnlock(mux);
}

void CarbonCondInit(carbon_cond_t *cond)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->condInit(cond);
}

void CarbonCondWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->condWait(cond, mux);
}

void CarbonCondSignal(carbon_cond_t *cond)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->condSignal(cond);
}

void CarbonCondBroadcast(carbon_cond_t *cond)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->condBroadcast(cond);
}

void CarbonBarrierInit(carbon_barrier_t *barrier, UInt32 count)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->barrierInit(barrier, count);
}

void CarbonBarrierWait(carbon_barrier_t *barrier)
{
   Tile *tile = Sim()->getTileManager()->getCurrentTile();
   tile->getSyncClient()->barrierWait(barrier);
}
