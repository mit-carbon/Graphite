/****************************************************
 * This is a test that will test dynamic thread     *
 * creation                                         *
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "capi.h"

// Functions executed by threads
void* thread_func(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   // Read in the command line arguments
   const unsigned int numThreads = 1;

   // Declare threads and related variables
   int threads[numThreads];

   for(unsigned int i = 0; i < 50; i++)
   {
      fprintf(stdout, "Spawning thread %d\n", i);

      for(unsigned int j = 0; j < numThreads; j++)
         threads[j] = CarbonSpawnThread(thread_func, NULL);

      for(unsigned int j = 0; j < numThreads; j++)
         CarbonJoinThread(threads[j]);
   }

   fprintf(stdout, "UserApplication: About to call carbon finish!\n");
}

void* thread_func(void *threadid)
{
//   CAPI_return_t rtnVal = CAPI_Initialize((int)threadid);
   return NULL;
}

