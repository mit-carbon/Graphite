/****************************************************
 * This is a test that will test barriers           *
 ****************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "carbon_user.h"
#include "capi.h"
#include "sync_api.h"

carbon_barrier_t my_barrier;

// Functions executed by threads
void* test_wait_barrier(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);
   const unsigned int num_threads = 5;
   carbon_thread_t threads[num_threads];

   fprintf(stderr, "Initializing barrier\n");

   CarbonBarrierInit(&my_barrier, num_threads);

   fprintf(stderr, "Spawning threads\n");

   for(unsigned int i = 0; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(test_wait_barrier, (void *) i);

   fprintf(stderr, "Joining threads\n");

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   fprintf(stderr, "Finished running barrier test!.\n");

   CarbonStopSim();
   return 0;
} // main ends

void* test_wait_barrier(void *threadid)
{
   fprintf(stderr, "Spawned thread %li\n", (long) threadid);

   for (unsigned int i = 0; i < 50; i++)
      CarbonBarrierWait(&my_barrier);

   return NULL;
}

