/****************************************************
 * This is a test that will test mutexes            *
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "carbon_user.h"

// Functions executed by threads
void* thread_func(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);

   fprintf(stderr, "In main()\n");

   // Read in the command line arguments
   const unsigned int numThreads = 1;

   // Declare threads and related variables
   carbon_thread_t threads[numThreads];

   CAPI_return_t rtnVal;
   rtnVal = CAPI_Initialize(0);

   fprintf(stderr, "Spawning...\n");

   for(unsigned int i = 0; i < numThreads; i++)
       threads[i] = CarbonSpawnThread(thread_func, (void *) (i + 1));

   fprintf(stderr, "Spawned, now joining ...\n");

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++)
      CarbonJoinThread(threads[i]);

   fprintf(stderr, "Joined, exiting ...\n");
   CarbonStopSim();
} // main ends

void* thread_func(void *threadid)
{
   CAPI_return_t rtnVal = CAPI_Initialize((int)threadid);
   fprintf(stderr, "In Spawned Thread..\n");
}

