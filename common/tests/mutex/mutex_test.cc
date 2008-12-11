/****************************************************
 * This is a test that will test mutexes            *
 ****************************************************/

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "user_api.h"

using namespace std;

carbon_mutex_t my_mux;

#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* test_mutex(void * threadid);

int main(int argc, char* argv[]){ // main begins

   carbonInit();

   // Read in the command line arguments
   const unsigned int numThreads = 1;

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

   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_create(&threads[i], &attr, test_mutex, (void *) i);

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_join(threads[i], NULL);

   cout << "quitting syscall server!" << endl;
   carbonFinish();

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends


void* test_mutex(void *threadid)
{
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;

  rtnVal = CAPI_Initialize((int)threadid);

  // Initialize local variables
  CAPI_rank(&tid);

  // Thread starts here

  // FIXME: This should be in the main thread or something.
  cerr << "Initializing the mutex." << endl;
  mutexInit(&my_mux);
  cerr << "After: " << hex << my_mux << endl;
  cerr << "Locking the mutex." << endl;
  mutexLock(&my_mux);
  cerr << "After: " << hex << my_mux << endl;
  cerr << "Unlocking the mutex." << endl;
  mutexUnlock(&my_mux);
  cerr << "After: " << hex << my_mux << endl;
  cerr << "Done with the mutex test." << endl;

  pthread_exit(NULL);
}

