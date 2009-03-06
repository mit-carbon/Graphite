#include "stdbool.h"
#include "stdio.h"
#include "pthread.h"
#include "thread_support.h"
#include "assert.h"

#ifdef __cplusplus
extern "C" 
{
#endif

// Support functions provided by the simulator
void CarbonGetThreadToSpawn(ThreadSpawnRequest **req)
{
   assert(false);
}

void CarbonThreadStart(ThreadSpawnRequest *req)
{
   assert(false);
}

void CarbonThreadExit()
{
   assert(false);
}

void *CarbonSpawnManagedThread(void *p)
{
   ThreadSpawnRequest *thread_info = (ThreadSpawnRequest *)p;

   CarbonThreadStart(thread_info);

   thread_info->func(thread_info->arg);

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
         pthread_t thread;
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

         pthread_create(&thread, &attr, CarbonSpawnManagedThread, req);
      }
      else
      {
         break;
      }
   }
   return NULL;
}

// This function spawns the thread spawner
int CarbonSpawnThreadSpawner()
{
   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, CarbonThreadSpawner, NULL);
   return 0;
}

#ifdef __cplusplus
}
#endif

