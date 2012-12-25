#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sched.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "thread_scheduler.h"
#include "tile_manager.h"
#include "tile.h"
#include "config_file.hpp"
#include "carbon_user.h"
#include "thread_support_private.h"

// FIXME: Pthread wrappers are untested.
int CarbonPthreadCreate(pthread_t *tid, int *attr, thread_func_t func, void *arg)
{
   LOG_ASSERT_WARNING(attr == NULL, "Attributes ignored in pthread_create.");
   LOG_ASSERT_ERROR(tid != NULL, "Null pointer passed to pthread_create.");

   *tid = CarbonSpawnThread(func, arg);
   return *tid >= 0 ? 0 : 1;
}

int CarbonPthreadJoin(pthread_t tid, void **pparg)
{
   LOG_ASSERT_WARNING(pparg == NULL, "Did not expect pparg non-NULL. It is ignored.");
   CarbonJoinThread(tid);
   return 0;
}

carbon_thread_t CarbonSpawnThread(thread_func_t func, void *arg)
{
   return Sim()->getThreadManager()->spawnThread(INVALID_TILE_ID, func, arg);
}

carbon_thread_t CarbonSpawnThreadOnTile(tile_id_t tile_id, thread_func_t func, void *arg)
{
   return Sim()->getThreadManager()->spawnThread(tile_id, func, arg);
}

bool CarbonSchedSetAffinity(thread_id_t thread_id, UInt32 cpusetsize, cpu_set_t* set)
{
   return Sim()->getThreadScheduler()->schedSetAffinity(thread_id, cpusetsize, set);
}

bool CarbonSchedGetAffinity(thread_id_t thread_id, UInt32 cpusetsize, cpu_set_t* set)
{
   return Sim()->getThreadScheduler()->schedGetAffinity(thread_id, cpusetsize, set);
}

void CarbonJoinThread(carbon_thread_t tid)
{
   Sim()->getThreadManager()->joinThread(tid);
}

// Support functions provided by the simulator
void CarbonThreadStart(ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->onThreadStart(req);
}

void CarbonThreadExit()
{
   Sim()->getThreadManager()->onThreadExit();
}

void CarbonGetThreadToSpawn(ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->getThreadToSpawn(req);
}

void CarbonDequeueThreadSpawnReq (ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->dequeueThreadSpawnReq (req);
}


void *CarbonSpawnManagedThread(void *)
{
   ThreadSpawnRequest thread_req;

   CarbonDequeueThreadSpawnReq (&thread_req);

   CarbonThreadStart(&thread_req);

   thread_req.func(thread_req.arg);

   CarbonThreadExit();
   
   return NULL;
}

// This function spawns the thread spawner
int CarbonSpawnThreadSpawner()
{
   setvbuf(stdout, NULL, _IONBF, 0 );

   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   CarbonPthreadAttrInitOtherAttr(&attr);
       
   pthread_create(&thread, &attr, CarbonThreadSpawner, NULL);

   return 0;
}

// This function will spawn threads provided by the sim
void *CarbonThreadSpawner(void *)
{
   ThreadSpawnRequest req = {-1, NULL, NULL, INVALID_CORE_ID, INVALID_THREAD_ID, Sim()->getConfig()->getCurrentThreadSpawnerCoreId(), 0};
   CarbonThreadStart (&req);

   while(1)
   {
      ThreadSpawnRequest req;
      
      // Wait for a spawn request
      CarbonGetThreadToSpawn(&req);

      if (req.func)
      {
         pthread_t thread;
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        
         CarbonPthreadAttrInitOtherAttr(&attr);

         pthread_create(&thread, &attr, CarbonSpawnManagedThread, NULL);
      }
      else
      {
         break;
      }
   }

   CarbonThreadExit();

   return NULL;
}

// This function initialized the pthread attributes
// Gets replaced while running with Pin
// attribute 'noinline' necessary to make the scheme work correctly with
// optimizations enabled; asm ("") in the body prevents the function from being
// optimized away
__attribute__((noinline)) void CarbonPthreadAttrInitOtherAttr(pthread_attr_t *attr)
{
   asm ("");
}
