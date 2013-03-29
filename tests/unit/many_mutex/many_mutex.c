/****************************************************
 * This is a test that will test the mutex multiple *
 * times                                            *
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include "capi.h"
#include "sync_api.h"

carbon_mutex_t my_mux1;
carbon_mutex_t my_mux2;
carbon_mutex_t my_mux3;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_many_mutex(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   // Read in the command line arguments
   const unsigned int numThreads = 5;

   fprintf(stderr, "TestManyMutex: Initting barrier.\n");
   // FIXME: shouldn't be hardcoding the barrier count here
   CarbonMutexInit(&my_mux1);
   CarbonMutexInit(&my_mux2);
   CarbonMutexInit(&my_mux3);
   fprintf(stderr, "TestManyMutex: Barrier Initialized.\n");
   
   // Declare threads and related variables
   pthread_t threads[numThreads];

#ifdef DEBUG
   printf("This is the function main()\n");
#endif

#ifdef DEBUG
   printf("Spawning threads\n");
#endif

   for (unsigned int i = 0; i < numThreads; i++)
      pthread_create(&threads[i], NULL, test_many_mutex, (void *) i);

   // Wait for all threads to complete
   for (unsigned int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

#ifdef DEBUG
   printf("This is the function main ending\n");
#endif
   
   return 0;
} // main ends

void* test_many_mutex(void *threadid)
{
   // Declare local variables
   int tid = (int) threadid;

   // Thread starts here

   for (int i = 0; i < 1000; i++)
   {
      CarbonMutexLock(&my_mux1);
      CarbonMutexLock(&my_mux2);
      CarbonMutexLock(&my_mux3);
      if (i % 100 == 0)
         fprintf(stderr, "Thread %d got %dth lock...\n", tid, i);
      CarbonMutexUnlock(&my_mux3);
      CarbonMutexUnlock(&my_mux2);
      CarbonMutexUnlock(&my_mux1);
   }

   return NULL;
}
