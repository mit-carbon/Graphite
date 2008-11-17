
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"
#include <sstream>

using namespace std;

struct cVector
{
		float x,y,z;
};

pthread_mutex_t write_lock;

int global_integer;
int* global_integer_ptr;

#define DEBUG 1

#ifdef DEBUG
pthread_mutex_t lock;
#endif


// Function executed by each thread
void* do_nothing(void *threadid);

//you can set pinsim to ONLY instrument this function
void instrument_me();

int main2(int argc, char* argv[]) {
  
  cout << "Begin Main " << endl;
   int tid;
   CAPI_Initialize(&tid);
//  cout << " After CAPI Initialization " << endl;
   
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


        pthread_join(threads[0], NULL);         
        pthread_join(threads[1], NULL);

#ifdef DEBUG
	cout << "End of execution" << endl << endl;
#endif
        
   return 0;
} // main ends

void BARRIER_DUAL_CORE(int tid)
{
	//this is a stupid barrier just for the test purposes
	int payload;

	cout << "BARRIER DUAL CORE for ID(" << tid << ")" << endl;
	if(tid==0) {
		CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
		CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
	} else {
		CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &payload, sizeof(int));
		CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &payload, sizeof(int));
	}
}

//spawned threads run this function
void* do_nothing(void *threadid)
{
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cout << "beginning of do_nothing function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
   
	int tid;
	cout << "start capi_init" << endl;
	CAPI_Initialize(&tid);
//	cout << "end   capi_init" << endl;

#ifdef DEBUG  
//   pthread_mutex_lock(&lock);
//   cout << "executing do_nothing function: " << tid << endl << endl;
//   pthread_mutex_unlock(&lock);
#endif
   
   int size = 10;
   global_integer = 10;
   global_integer_ptr = &global_integer;
//		stringstream ss;
//		ss << "\n\nsize addr: " << &size << endl << endl;
//		CAPI_Print(ss.str());
//		BARRIER_DUAL_CORE(tid);
//		instrument_me( );
		
//		pthread_mutex_lock(&lock);
//		cout << "Core: " << tid << " finished instrumenting." << endl;
//		pthread_mutex_unlock(&lock);
//   }
   
	CAPI_Finish(tid);
//	cerr << "finished running... and now pthread exit!" << endl;
	pthread_exit(NULL);  
}

//int instrument_me(int tid, int* ptr) 
void instrument_me()
{
//	cout << "Greetings I wonder if this will cause any crashes or anything else interesting?1" << endl;
int size = 128;
char array[size];

	//4 bytes first line (at the end), 4 bytes overflow onto second line
	int addr = (((int)array) / 32) * 32 + 30;
	*((UINT64*) addr)  = 0xFFFF;
   UINT64 x = *((UINT64*) addr);

//   cout << hex << "x = " << ((UINT64) x) << endl;
}

