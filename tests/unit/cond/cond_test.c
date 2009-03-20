/****************************************************
 * This is a test that will test condition variables*
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "capi.h"
#include "mcp_api.h"
#include "sync_api.h"

carbon_mutex_t my_mux;
carbon_cond_t my_cond;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_wait_cond(void * threadid);
void* test_signal_cond(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim();

   // Read in the command line arguments
   const unsigned int numThreads = 2;

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

   pthread_create(&threads[0], &attr, test_wait_cond, (void *) 0);
   pthread_create(&threads[1], &attr, test_signal_cond, (void *) 1);

   // Wait for all threads to complete
   for (unsigned int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

   printf("quitting syscall server!\n");
   quitMCP();

#ifdef DEBUG
   printf("This is the function main ending\n");
#endif
   pthread_exit(NULL);

   CarbonStopSim();
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

void* test_signal_cond(void *threadid)
{
   usleep(500000);
   // Declare local variables
   int tid;
   CAPI_return_t rtnVal;

   rtnVal = CAPI_Initialize((int)threadid);

   // Initialize local variables
   CAPI_rank(&tid);

   // Make sure that the signal comes after the wait
   wait_some();

   // Thread starts here
   fprintf(stderr, "UserSignal: Cond Signaling.");
   CarbonCondSignal(&my_cond);
   fprintf(stderr, "UserSignal: Cond Signaled.");
   CarbonMutexLock(&my_mux);
   fprintf(stderr, "UserSignal: Mutex locked after signal.");
   CarbonMutexUnlock(&my_mux);
   fprintf(stderr, "UserSignal: Signal thread done.");

   pthread_exit(NULL);
}

void* test_wait_cond(void *threadid)
{
   // Declare local variables
   int tid;
   CAPI_return_t rtnVal;

   rtnVal = CAPI_Initialize((int)threadid);

   // Initialize local variables
   CAPI_rank(&tid);

   // Thread starts here

   // FIXME: This should be in the main thread or something.
   fprintf(stderr, "UserWait: Initting mutex.");
   CarbonMutexInit(&my_mux);
   fprintf(stderr, "UserWait: Initting cond.");
   CarbonCondInit(&my_cond);

   fprintf(stderr, "UserWait: Locking mux.");
   CarbonMutexLock(&my_mux);
   fprintf(stderr, "UserWait: Cond wait.");
   CarbonCondWait(&my_cond, &my_mux);
   fprintf(stderr, "UserWait: Cond done.");

   CarbonMutexUnlock(&my_mux);
   fprintf(stderr, "UserWait: test_wait_cond mutex unlock done.");

   pthread_exit(NULL);
}

