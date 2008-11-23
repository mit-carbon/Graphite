
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"
#include "mcp_api.h"

using namespace std;

pthread_mutex_t write_lock;

#define DEBUG

#ifdef DEBUG
pthread_mutex_t lock;
#endif


// Function executed by each thread
void* ping(void *threadid);
void* pong(void *threadid);

int main(int argc, char* argv[]){ // main begins

	initMCP();
	// Declare threads and related variables
	pthread_t threads[2];
	pthread_attr_t attr;
	
#ifdef DEBUG
	cerr << "This is the function main()" << endl;
#endif
	// Initialize global variables

#ifdef DEBUG
	cerr << "Initializing thread structures" << endl << endl;
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);
#ifdef DEBUG
	cerr << "Spawning threads" << endl << endl;
#endif
   
	
	
	pthread_mutex_lock(&lock);
	cerr << "Creating Thread#0." << endl << endl;
   pthread_mutex_unlock(&lock);
	
   pthread_create(&threads[0], &attr, ping, (void *) 0);

   
	pthread_mutex_lock(&lock);
	cerr << "Creating Thread#1." << endl << endl;
   pthread_mutex_unlock(&lock);
	
   pthread_create(&threads[1], &attr, pong, (void *) 1);

   
	pthread_mutex_lock(&lock);
	cerr << "Starter thread is waiting for the other threads to join." << endl << endl;
   pthread_mutex_unlock(&lock);
	

	while(1);

	// Wait for all threads to complete
   pthread_join(threads[0], NULL);         
   pthread_join(threads[1], NULL);
	
	cerr << "Finished running PingPong!." << endl << endl;

	quitMCP();

   return 0;
} // main ends



void* ping(void *threadid)
{
   int tid;
	int junk;


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing ping function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


   CAPI_Initialize_FreeRank(&tid);

	
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "TID = " << tid << endl;
   pthread_mutex_unlock(&lock);
#endif


   pthread_mutex_lock(&lock);
	cerr << "ping says finished capi_init" << endl;
   pthread_mutex_unlock(&lock);


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing ping function with <tid,!tid>= <" << tid << "," << !tid << ">" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int));

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "ping sent to pong" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "ping received from pong" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif



	CAPI_Finish(tid);

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "ping finished CAPI_Finish" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

   pthread_exit(NULL);  
   // return 0;
}

void* pong(void *threadid)
{
   int tid;
	int junk;
/*
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing pong function" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
*/
   CAPI_Initialize_FreeRank(&tid);
 
/*
   pthread_mutex_lock(&lock);
	cerr << "pong finished with capi_init" << endl;
   pthread_mutex_unlock(&lock);

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing pong function with <tid,!tid>= <" << tid << "," << !tid << ">" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
*/
 
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int)); 

/*
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "pong sent to ping" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
*/

   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  

/*
#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "pong received from ping" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
*/

   CAPI_Finish(tid);
	pthread_exit(NULL);  
   // return 0;
}
