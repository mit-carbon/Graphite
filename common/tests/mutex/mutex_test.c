/****************************************************
 * This is a test that will test mutexes            *
 ****************************************************/

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <pthread.h>

#include "sync_api.h"
#include "capi.h"

carbon_mutex_t my_mux;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_mutex(void * threadid);

int main(int argc, char* argv[])  // main begins
{

   // Read in the command line arguments
   const unsigned int numThreads = 1;

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
   printf("Spawning threads");
#endif

   for (unsigned int i = 0; i < numThreads; i++)
      pthread_create(&threads[i], &attr, test_mutex, (void *) i);

   // Wait for all threads to complete
   for (unsigned int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

   printf("quitting syscall server!\n");

#ifdef DEBUG
   printf("This is the function main ending\n");
#endif
   pthread_exit(NULL);

} // main ends


void* test_mutex(void *threadid)
{
   // Declare local variables
   int tid;
   CAPI_return_t rtnVal;

   carbonInitializeThread();
   rtnVal = CAPI_Initialize((int)threadid);

   // Initialize local variables
   CAPI_rank(&tid);

   // Thread starts here

   // FIXME: This should be in the main thread or something.
   fprintf(stderr, "Initializing the mutex.\n");
   mutexInit(&my_mux);
   fprintf(stderr, "After: %x\n", my_mux);
   fprintf(stderr, "Locking the mutex.\n");
   mutexLock(&my_mux);
   fprintf(stderr, "After: %x\n", my_mux);
   fprintf(stderr, "Unlocking the mutex.\n");
   mutexUnlock(&my_mux);
   fprintf(stderr, "After: %x\n", my_mux);
   fprintf(stderr, "Done with the mutex test.\n");

   pthread_exit(NULL);
}

