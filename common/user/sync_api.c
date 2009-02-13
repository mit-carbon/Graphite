#include "sync_api.h"
#include <stdio.h>
#include <assert.h>

void mutexInit(carbon_mutex_t *mux)
{
   fprintf(stderr, "Made it to the dummy mutexInit() function.\n");
   assert(false);
}

void mutexLock(carbon_mutex_t *mux)
{
   fprintf(stderr, "Made it to the dummy mutexLock() function.\n");
   assert(false);
}

void mutexUnlock(carbon_mutex_t *mux)
{
   fprintf(stderr, "Made it to the dummy mutexUnlock() function.\n");
   assert(false);
}

void condInit(carbon_cond_t *cond)
{
   fprintf(stderr, "Made it to the dummy condInit() function.\n");
   assert(false);
}

void condWait(carbon_cond_t *cond, carbon_mutex_t *mux)
{
   fprintf(stderr, "Made it to the dummy condWait() function.\n");
   assert(false);
}

void condSignal(carbon_cond_t *cond)
{
   fprintf(stderr, "Made it to the dummy condSignal() function.\n");
   assert(false);
}

void condBroadcast(carbon_cond_t *cond)
{
   fprintf(stderr, "Made it to the dummy condBroadcast() function.\n");
   assert(false);
}

void barrierInit(carbon_barrier_t *barrier, unsigned int count)
{
   fprintf(stderr, "Made it to the dummy barrierInit() function.\n");
   assert(false);
}

void barrierWait(carbon_barrier_t *barrier)
{
   fprintf(stderr, "Made it to the dummy barrierWait() function.\n");
   assert(false);
}

bool isMutexValid(carbon_mutex_t *mux)
{
   return(!(*mux == -1));
}

bool isCondValid(carbon_cond_t *cond)
{
   return(!(*cond == -1));
}

bool isBarrierValid(carbon_barrier_t *barrier)
{
   return(!(*barrier == -1));
}
