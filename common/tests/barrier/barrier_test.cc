/****************************************************
 * This is a test that will test barriers           *
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

carbon_barrier_t my_barrier;

#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_wait_barrier(void * threadid);

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

   for (unsigned int i = 0; i < numThreads; i++)
     pthread_create(&threads[i], &attr, test_wait_barrier, (void *) i);

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_join(threads[i], NULL);

   cout << "Quitting syscall server!" << endl;
   quitMCP();

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends

void wait_some()
{
   int j = 0;
   for(unsigned int i = 0; i < 200000; i++)
   {
      j += i;
      asm volatile("nop");
   }
}

void* test_wait_barrier(void *threadid)
{
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;

  rtnVal = CAPI_Initialize((int)threadid);

  // Initialize local variables
  CAPI_rank(&tid);

  if(tid != (int)threadid)
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
      sleep(1);
    }


  if(tid == 1)
  {
     fprintf(stderr, "UserWait: THREAD (%d).\n", tid);
     wait_some();
  }

  fprintf(stderr, "UserWait(%d): Waiting for barrier.\n", (int)threadid);
  barrierWait(&my_barrier);
  fprintf(stderr, "UserWait(%d): barrier done.\n", (int)threadid);

  pthread_exit(NULL);
}

