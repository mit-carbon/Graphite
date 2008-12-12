/****************************************************
 * This is a test that will test condition variables*
 ****************************************************/

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "capi.h"
#include "mcp_api.h"
#include "sync_api.h"
#include <stdio.h>

using namespace std;

carbon_mutex_t my_mux;
carbon_cond_t my_cond;

#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_wait_cond(void * threadid);
void* test_broadcast_cond(void * threadid);

int main(int argc, char* argv[]){ // main begins

   initMCP();

   // Read in the command line arguments
   const unsigned int numThreads = 5;

   // Declare threads and related variables
   pthread_t threads[numThreads];
   pthread_attr_t attr;
	
#ifdef DEBUG
   cout << "This is the function main()" << endl;
#endif

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

#ifdef DEBUG
   cout << "Spawning threads" << endl;
#endif

   for (unsigned int i = 0; i < numThreads - 1; i++)
     pthread_create(&threads[i], &attr, test_wait_cond, (void *) i);
   pthread_create(&threads[numThreads-1], &attr, test_broadcast_cond, (void *) NULL);

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_join(threads[i], NULL);

   cout << "quitting syscall server!" << endl;
   quitMCP();

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends

void* test_broadcast_cond(void *threadid)
{
  sleep(3);
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;

  rtnVal = CAPI_Initialize((int)threadid);

  // Initialize local variables
  CAPI_rank(&tid);

  // Thread starts here
  fprintf(stderr, "UserBroadcast: Cond broadcasting.\n");
  condBroadcast(&my_cond);
  fprintf(stderr, "UserBroadcast: Cond broadcasted.\n");
  mutexLock(&my_mux);
  fprintf(stderr, "UserBroadcast: Mutex locked after broadcast.\n");
  mutexUnlock(&my_mux);
  fprintf(stderr, "UserBroadcast: Broadcast thread done.\n");

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
  if ((int)threadid == 0)
    {
      fprintf(stderr, "UserWait: Initting mutex.\n");
      mutexInit(&my_mux);
      fprintf(stderr, "UserWait: Initting cond.\n");
      condInit(&my_cond);
    }

  sleep(1);

  fprintf(stderr, "UserWait: Locking mux.\n");
  mutexLock(&my_mux);
  fprintf(stderr, "UserWait: Cond wait.\n");
  condWait(&my_cond, &my_mux);
  fprintf(stderr, "UserWait: Cond done.\n");

  mutexUnlock(&my_mux);
  fprintf(stderr, "UserWait: test_wait_cond mutex unlock done.\n");

  pthread_exit(NULL);
}

