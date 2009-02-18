/****************************************************
 * This is a test that will test barriers           *
 ****************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include "capi.h"
#include "sync_api.h"

carbon_barrier_t my_barrier;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_wait_barrier(void * threadid);

int main(int argc, char* argv[])  // main begins
{

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
      pthread_create(&threads[i], &attr, test_wait_barrier, (void *) i);

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
   for (unsigned int i = 0; i < 20000; i++)
   {
      j += i;
   }
   return j;
}

void* test_wait_barrier(void *threadid)
{
   // Declare local variables
   int tid, i;
   CAPI_return_t rtnVal;

   carbonInitializeThread();
   rtnVal = CAPI_Initialize((int)threadid);

   // Initialize local variables
   CAPI_rank(&tid);

   if (tid != (int)threadid)
      fprintf(stderr, "UserWait(%d != %d): tid didn't match threadid.\n", tid, (int)threadid);

   // Thread starts here

   // FIXME: This should be in the main thread or something.
   if ((int)threadid == 0)
   {
      fprintf(stderr, "UserWait(%d): Initting barrier.\n", (int)threadid);
      // FIXME: shouldn't be hardcoding the barrier count here
      barrierInit(&my_barrier, 5);
      fprintf(stderr, "UserWait(%d): Barrier Initialized.\n", (int)threadid);
   }
   else
   {
      sleep(5);
   }


   for (i = 0; i < 50; i++)
   {
      if (tid == 1)
      {
         fprintf(stderr, "UserWait: THREAD (%d).\n", i);
         wait_some();
      }

      barrierWait(&my_barrier);
   }

   pthread_exit(NULL);
}

