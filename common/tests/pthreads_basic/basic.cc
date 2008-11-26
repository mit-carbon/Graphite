#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"
#include "mcp_api.h"
#include <sstream>

using namespace std;

pthread_mutex_t write_lock;

#define SIZE (100)

int global_integer;
int* global_integer_ptr;
int g_array[SIZE];

unsigned int coreCount;

#define DEBUG 1

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Function executed by each thread
void* thread_main(void *threadid);

//you can set pinsim to ONLY instrument this function
void instrument_me();

int main(int argc, char* argv[]){ // main begins

   // Read in the command line arguments
	if(argc != 3) {
		cout << "invalid command line options. the correct format is:" << endl;
		cout << "basic -n num_of_threads" << endl;
		exit(EXIT_FAILURE);
	}
	else if((strcmp(argv[1], "-n\0") == 0)){
		coreCount = atoi(argv[2]);
	}
	else {
		cout << "invalid command line options. the correct format is:" << endl;
		cout << "basic -n num_of_threads" << endl;
		exit(EXIT_FAILURE);
	}

	// declare threads and related variables
	pthread_t threads[coreCount];
	pthread_attr_t attr;
	
	initMCP();
#ifdef DEBUG
	cout << "This is the function main()" << endl;
	cout << "Initializing thread structures" << endl << endl;
	pthread_mutex_init(&lock, NULL);
#endif
	
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   for(unsigned int i=0; i < coreCount; i++) {
		pthread_create(&threads[i], &attr, thread_main, (void *) i);    
	}

	cerr << "finished starting threads" << endl;

   for(unsigned int i=0; i < coreCount; i++) {
		pthread_join(threads[i], NULL);         
	}
	
#ifdef DEBUG
	cout << "End of execution" << endl << endl;
#endif

	quitMCP();
        
   return 0;
} // main ends

void barrier(int tid)
{
	int payload; //unused

	if(tid==0) 
	{
		//gather all receiver messages, then send a continue to all cores
		for(unsigned int i=1; i < coreCount; i++) {
			CAPI_message_receive_w(i, 0, (char *) &payload, sizeof(int));
		}
		for(unsigned int i=1; i < coreCount; i++) {
			CAPI_message_send_w(0, i, (char *) &payload, sizeof(int));
		}
	} else 
	{
		CAPI_message_send_w(tid, 0, (char *) &payload, sizeof(int));
		CAPI_message_receive_w(0, tid, (char *) &payload, sizeof(int));
	}
}

//spawned threads run this function
void* thread_main(void *threadid)
{
	int tid;
	printf(" Thread Main\n");
//	CAPI_Initialize(&tid);
	CAPI_Initialize_FreeRank(&tid);

	if(tid==0) printf("after capi init\n");
   global_integer = 10;
   global_integer_ptr = &global_integer;
  
	for(int i=0; i < SIZE; i++) {
		if(tid==0 && (i % 10) == 0 ) printf("Loop: %d\n", i);
		g_array[(i + tid*(SIZE/coreCount)) % SIZE] = g_array[((i + tid*(SIZE/coreCount) + (INT32) (SIZE/4)) % SIZE)];
	}
   
	CAPI_Finish(tid);
	pthread_exit(NULL);  
}

void instrument_me()
{
	int size = 128;
	char array[size];

	//4 bytes first line (at the end), 4 bytes overflow onto second line
	unsigned int addr = ((((unsigned int) array) >> 5) << 5) + 31;	// Ensure that there is no segmentation fault
	*((unsigned int*) addr)  = 0xFFFF;
   unsigned int x = *((unsigned int*) addr);

}

