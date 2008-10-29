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

void* test_wait_barrier(void *threadid)
{
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;

  rtnVal = CAPI_Initialize(&tid);

  // Initialize local variables
  CAPI_rank(&tid);

  // Thread starts here

  // FIXME: This should be in the main thread or something.
  if ((int)threadid == 0)
    {
      fprintf(stderr, "UserWait: Initting barrier.\n");
      // FIXME: shouldn't be hardcoding the barrier count here
      barrierInit(&my_barrier, 5);
    }
  else
    {
      sleep(1);
    }


  fprintf(stderr, "UserWait: Waiting for barrier.\n");
  barrierWait(&my_barrier);
  fprintf(stderr, "UserWait: barrier done.\n");

  pthread_exit(NULL);
}

