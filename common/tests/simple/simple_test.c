/****************************************************
 * This is a test that will test mutexes            *
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

int main(int argc, char* argv[]){ // main begins

    int proc = atoi(argv[1]);
    fprintf(stderr, "Process: %d\n", proc);

   // Read in the command line arguments
   const unsigned int numThreads = 1;

   // Declare threads and related variables
//   pthread_t threads[numThreads];
//   pthread_attr_t attr;
	
   // Initialize threads and related variables
//   pthread_attr_init(&attr);
//   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   // Initialize the main thread as a core
   if(proc == 0)
   {
      fprintf(stderr, "Calling capi initialize!\n");
      int tid;
      CAPI_return_t rtnVal;
      rtnVal = CAPI_Initialize(0);

      // Initialize local variables
      CAPI_rank(&tid);
   }
   else
   {
      fprintf(stderr, "Calling thread_func(1)!\n");
      thread_func(1);
      /*
      for(unsigned int i = 0; i < numThreads; i++) 
         pthread_create(&threads[i], &attr, thread_func, (void *) (i + 1));

      // Wait for all threads to complete
      for(unsigned int i = 0; i < numThreads; i++) 
         pthread_join(threads[i], NULL);
         */
   }

   printf("UserApplication: About to call carbon finish!\n");

//   pthread_exit(NULL);
} // main ends


void* thread_func(void *threadid)
{
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;

  rtnVal = CAPI_Initialize((int)threadid);

  // Initialize local variables
  CAPI_rank(&tid);

  // Thread starts here
  printf("UserThread: CAPI Rank: %d\n", tid);

//  pthread_exit(NULL);
}

