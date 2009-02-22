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

#include "capi.h"

// Functions executed by threads
void* thread_func(void * threadid);

int main(int argc, char* argv[])  // main begins
{
    CarbonInitializeThread();

   // Read in the command line arguments
   const unsigned int numThreads = 1;

   // Declare threads and related variables
   pthread_t threads[numThreads];
   pthread_attr_t attr;

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   unsigned int i;
   for(unsigned int i = 0; i < 50; i++)
   {
      for(unsigned int j = 0; j < numThreads; j++)
         pthread_create(&threads[j], &attr, thread_func, (void *) (j + 1));

      // Wait for all threads to complete
      for(unsigned int j = 0; j < numThreads; j++)
         pthread_join(threads[j], NULL);
   }

   printf("UserApplication: About to call carbon finish!\n");

} // main ends

void* thread_func(void *threadid)
{
   CarbonInitializeThread();
   CAPI_return_t rtnVal = CAPI_Initialize((int)threadid);
   return NULL;
}

