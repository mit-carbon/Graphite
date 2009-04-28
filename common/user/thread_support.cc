#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <sched.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
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
   return Sim()->getThreadManager()->spawnThread(func, arg);
}

void CarbonJoinThread(carbon_thread_t tid)
{
   Sim()->getThreadManager()->joinThread(tid);
}

// Support functions provided by the simulator
void CarbonGetThreadToSpawn(ThreadSpawnRequest **req)
{
   Sim()->getThreadManager()->getThreadToSpawn(req);
}

void CarbonThreadStart(ThreadSpawnRequest *req)
{
   Sim()->getThreadManager()->onThreadStart(req);
}

void CarbonThreadExit()
{
   Sim()->getThreadManager()->onThreadExit();
}

void *CarbonSpawnManagedThread(void *p)
{
   ThreadSpawnRequest *req = (ThreadSpawnRequest *)p;

   void* (*func)(void*) = req->func;
   void* arg = req->arg;

   CarbonThreadStart(req);

   func(arg);

   CarbonThreadExit();
   return NULL;
}

// This function will spawn threads provided by the sim
void *CarbonThreadSpawner(void *p)
{
   while(1)
   {
      ThreadSpawnRequest *req;

      // Wait for a spawn
      CarbonGetThreadToSpawn(&req);

      if(req->func)
      {
         CarbonThreadSpawnerSpawnThread(req);
      }
      else
      {
         break;
      }
   }
   return NULL;
}

void CarbonThreadSpawnerSpawnThread(ThreadSpawnRequest *req)
{
   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, CarbonSpawnManagedThread, req);
}

// This function spawns the thread spawner
int CarbonSpawnThreadSpawner()
{
   setvbuf( stdout, NULL, _IONBF, 0 );

   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, CarbonThreadSpawner, NULL);
   return 0;
}
