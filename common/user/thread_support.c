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
void CarbonGetThreadToSpawn(thread_func_t *func, void **p, int *core_id)
{
   assert(false);
}

void CarbonThreadStart(int core_id)
{
   assert(false);
}

void CarbonThreadExit()
{
   assert(false);
}

void *CarbonSpawnManagedThread(void *p)
{
   carbon_thread_info_t *thread_info = (carbon_thread_info_t *)p;

   CarbonThreadStart(thread_info->core_id);

   thread_info->func(thread_info->arg);

   CarbonThreadExit();
}


// This function will spawn threads provided by the sim
void *CarbonThreadSpawner(void *p)
{
   while(1)
   {
      thread_func_t func;
      void *arg;
      int core_id;

      // Wait for a spawn
      CarbonGetThreadToSpawn(&func, &arg, &core_id);

      if(func)
      {
         pthread_t thread;
         pthread_attr_t attr;
         pthread_attr_init(&attr);
         pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

         carbon_thread_info_t thread_info = { func, p, core_id };

         pthread_create(&thread, &attr, CarbonSpawnManagedThread, &thread_info);
      }
      else
      {
         break;
      }
   }
}

// This function spawns the thread spawner
int CarbonSpawnThreadSpawner()
{
   pthread_t thread;
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
   pthread_create(&thread, &attr, CarbonThreadSpawner, NULL);
}

#ifdef __cplusplus
}
#endif

