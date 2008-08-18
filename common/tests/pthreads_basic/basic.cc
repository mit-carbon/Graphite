
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"

using namespace std;

pthread_mutex_t write_lock;

#define DEBUG 1

#ifdef DEBUG
pthread_mutex_t lock;
#endif


// Function executed by each thread
void* do_nothing(void *threadid);


int main2(int argc, char* argv[]) {
  
  cout << "Begin Main " << endl;
   int tid;
   CAPI_Initialize(&tid);
  cout << " After CAPI Initialization " << endl;
   
   int size = 1;
   int array[size];

   for(int i=0; i < size; i++) {
      array[i] = i;
   }

   return 0;
}


int main(int argc, char* argv[]){ // main begins

	// Declare threads and related variables
	pthread_t threads[2];
	pthread_attr_t attr;
	
#ifdef DEBUG
	cout << "This is the function main()" << endl;
#endif
	// Initialize global variables

#ifdef DEBUG
	cout << "Initializing thread structures" << endl << endl;
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);

#ifdef DEBUG
	cout << "Spawning threads" << endl << endl;
#endif

      pthread_create(&threads[0], &attr, do_nothing, (void *) 0);    
      pthread_create(&threads[1], &attr, do_nothing, (void *) 1);    

#ifdef DEBUG
   pthread_mutex_lock(&lock);
	cout << "Waiting to join" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

	// Wait for all threads to complete
        pthread_join(threads[0], NULL);         
        pthread_join(threads[1], NULL);

#ifdef DEBUG
	cout << "End of execution" << endl << endl;
#endif
        
   return 0;
} // main ends

//spawned threads run this function
void* do_nothing(void *threadid)
{
	int tid;
	CAPI_Initialize(&tid);

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cout << "executing do_nothing function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
   pthread_exit(NULL);  
   // return 0;
}

