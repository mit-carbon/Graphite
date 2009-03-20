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
#include "sync.h"

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
   CarbonStartSim();

   // Read in the command line arguments
   const unsigned int numThreads = 5;

   // Declare threads and related variables
   pthread_t threads[numThreads];
   pthread_attr_t attr;

#ifdef DEBUG
   printf("This is the function main()\n");
#endif

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

#ifdef DEBUG
   printf("Spawning threads\n");
#endif

   for (unsigned int i = 0; i < numThreads; i++)
      pthread_create(&threads[i], &attr, test_many_mutex, (void *) i);

   // Wait for all threads to complete
   for (unsigned int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

   printf("Quitting syscall server!\n");

#ifdef DEBUG
   printf("This is the function main ending\n");
#endif
   pthread_exit(NULL);

} // main ends

int wait_some()
{
   int j = 0;
   for (unsigned int i = 0; i < 200000; i++)
   {
      j += i;
   }
   return j;
}

void* test_many_mutex(void *threadid)
{
   // Declare local variables
   int tid, i;
   CAPI_return_t rtnVal;

   rtnVal = CAPI_Initialize((int)threadid);

   // Initialize local variables
   CAPI_rank(&tid);

   if (tid != (int)threadid)
      fprintf(stderr, "TestManyMutex(%d != %d): tid didn't match threadid.\n", tid, (int)threadid);

   // Thread starts here

   // FIXME: This should be in the main thread or something.
   if ((int)threadid == 0)
   {
      fprintf(stderr, "TestManyMutex(%d): Initting barrier.\n", (int)threadid);
      // FIXME: shouldn't be hardcoding the barrier count here
      CarbonMutexInit(&my_mux1);
      CarbonMutexInit(&my_mux2);
      CarbonMutexInit(&my_mux3);
      fprintf(stderr, "TestManyMutex(%d): Barrier Initialized.\n", (int)threadid);
   }
   else
   {
      sleep(1);
   }

   for (i = 0; i < 1000; i++)
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

   CarbonStopSim();
}

