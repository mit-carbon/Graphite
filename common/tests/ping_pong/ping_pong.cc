
#include <iostream>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"
#include "user_api.h"
//#include "mcp_api.h"

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

   carbonInit();

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
   
	
#ifdef DEBUG	
	pthread_mutex_lock(&lock);
	cerr << "Creating Thread#0." << endl << endl;
   pthread_mutex_unlock(&lock);
#endif
	
   pthread_create(&threads[0], &attr, ping, (void *) 0);

#ifdef DEBUG
	pthread_mutex_lock(&lock);
	cerr << "Creating Thread#1." << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

   pthread_create(&threads[1], &attr, pong, (void *) 1);

#ifdef DEBUG 
	pthread_mutex_lock(&lock);
	cerr << "Starter thread is waiting for the other threads to join." << endl << endl;
   pthread_mutex_unlock(&lock);
#endif

	// Wait for all threads to complete
   pthread_join(threads[0], NULL);         
   pthread_join(threads[1], NULL);
	
	cerr << "Finished running PingPong!." << endl << endl;

   carbonFinish();
   return 0;

} // main ends


void* ping(void *threadid)
{
	int tid = (int)threadid;
   CAPI_Initialize((int)threadid);

	/*
	int tid;
	CAPI_Initialize_FreeRank(&tid);
	*/
	
	int junk;

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing ping function with <tid,!tid>= <" << tid << "," << !tid << ">" << endl;
   pthread_mutex_unlock(&lock);
#endif
 
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int));  

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "ping sent to pong" << endl;
   pthread_mutex_unlock(&lock);
#endif
   
	
	CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "ping received from pong" << endl;
   pthread_mutex_unlock(&lock);
#endif


   pthread_exit(NULL);  
   // return 0;
}

void* pong(void *threadid)
{
   
	int tid = (int)threadid;
   CAPI_Initialize((int)threadid);
	
	/*
	int tid;
	CAPI_Initialize_FreeRank(&tid);
	*/

	int junk;


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "executing pong function with <tid,!tid>= <" << tid << "," << !tid << ">" << endl;
   pthread_mutex_unlock(&lock);
#endif
 
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int)); 


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "pong sent to ping" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   cerr << "pong received from ping" << endl << endl;
   pthread_mutex_unlock(&lock);
#endif


	pthread_exit(NULL);  
   // return 0;
}

