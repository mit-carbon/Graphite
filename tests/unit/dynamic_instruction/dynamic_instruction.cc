#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "carbon_user.h"
#include "capi.h"
#include "sync_api.h"

carbon_barrier_t global_barrier;
carbon_mutex_t global_mutex;
UInt64 global_start_time = 0;
UInt64 global_end_time = UINT64_MAX_;

// Functions executed by threads
void* test_dynamic_instruction(void *thread_arg);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);
   
   const unsigned int num_threads = 64;
   carbon_thread_t threads[num_threads];
   int thread_args[num_threads];

   printf("Initializing barrier\n");

   CarbonBarrierInit(&global_barrier, num_threads);
   CarbonMutexInit(&global_mutex);

   printf("Spawning threads\n");

   for (unsigned int i = 1; i < num_threads; i++)
   {
      thread_args[i] = i;
      threads[i] = CarbonSpawnThread(test_dynamic_instruction, (void *) &thread_args[i]);
   }
   thread_args[0] = 0;
   test_dynamic_instruction(&thread_args[0]);

   for(unsigned int i = 1; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   printf("Joined threads\n");
  
   printf("Global Barrier Acquire Time (%llu), Global Barrier Release Time(%llu)\n", global_start_time, global_end_time);

   if (global_start_time > global_end_time)
   {
      fprintf(stderr, "FAILED: Global Barrier Acquire Time(%llu) > Global Barrier Release Time(%llu)\n",
            global_start_time, global_end_time);
      exit(-1);
   }
   CarbonStopSim();
   
   printf("SUCCESS: Finished running dynamic instruction test!.\n");
   
   return 0;
} // main ends

void* test_dynamic_instruction(void *thread_arg)
{
   int thread_id = *((int*) thread_arg);
   fprintf(stderr, "Spawned thread (%i)\n", thread_id);

   UInt64 start_time = CarbonGetTime();
   CarbonBarrierWait(&global_barrier);
   UInt64 end_time = CarbonGetTime();

   printf("Thread (%i): Start waiting at (%llu), end waiting at (%llu)\n", thread_id, start_time, end_time);
   
   CarbonMutexLock(&global_mutex);
   
   if (start_time >= global_start_time)
      global_start_time = start_time;
   if (end_time <= global_end_time)
      global_end_time = end_time;
   
   CarbonMutexUnlock(&global_mutex);
   return NULL;
}

