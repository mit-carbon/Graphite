
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "capi.h"

pthread_mutex_t write_lock;

#define DEBUG

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Function executed by each thread
void* ping(void *threadid);
void* pong(void *threadid);

int main(int argc, char* argv[]){ // main begins

	// Declare threads and related variables
	pthread_t threads[2];
	pthread_attr_t attr;
	
#ifdef DEBUG
	fprintf(stderr, "This is the function main()\n");
#endif
	// Initialize global variables

#ifdef DEBUG
	fprintf(stderr, "Initializing thread structures\n\n");
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);
#ifdef DEBUG
	fprintf(stderr, "Spawning threads\n\n");
#endif
   
	
#ifdef DEBUG	
	pthread_mutex_lock(&lock);
	fprintf(stderr, "Creating Thread#0.\n\n");
   pthread_mutex_unlock(&lock);
#endif
	
   pthread_create(&threads[0], &attr, ping, (void *) 0);

#ifdef DEBUG
	pthread_mutex_lock(&lock);
	fprintf(stderr, "Creating Thread#1.\n\n");
   pthread_mutex_unlock(&lock);
#endif

   pthread_create(&threads[1], &attr, pong, (void *) 1);

#ifdef DEBUG 
	pthread_mutex_lock(&lock);
	fprintf(stderr, "Starter thread is waiting for the other threads to join.\n\n");
   pthread_mutex_unlock(&lock);
#endif

	// Wait for all threads to complete
   pthread_join(threads[0], NULL);         
   pthread_join(threads[1], NULL);
	
	fprintf(stderr, "Finished running PingPong!.\n\n");

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
   fprintf(stderr, "executing ping function with <tid,!tid>= <%d,%d>\n", tid, !tid);
   pthread_mutex_unlock(&lock);
#endif
 
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int));  

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   fprintf(stderr, "ping sent to pong\n");
   pthread_mutex_unlock(&lock);
#endif
   
	
	CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  

#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   fprintf(stderr, "ping received from pong\n");
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
   fprintf(stderr, "executing pong function with <tid,!tid>= <%d,%d>\n", tid, !tid);
   pthread_mutex_unlock(&lock);
#endif
 
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int)); 


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   fprintf(stderr, "pong sent to ping\n\n");
   pthread_mutex_unlock(&lock);
#endif


   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));  


#ifdef DEBUG  
   pthread_mutex_lock(&lock);
   fprintf(stderr, "pong received from ping\n\n");
   pthread_mutex_unlock(&lock);
#endif


	pthread_exit(NULL);  
   // return 0;
}

