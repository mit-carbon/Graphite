/********************************************************
 * This test will send arrays values to a sub-thread to *
 * be multiplied.                                       *
 *                          - Charles Gruenwald         *
 *                          12/12/2008                  *
 *******************************************************/

#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "user_api.h"

unsigned int numThreads = 8;

#define SIZE 1024

// Function executed by each thread
void* worker(void * threadid);

int main(int argc, char* argv[]){ // main begins

   carbonInit();
	
	
	pthread_t* threads;
	pthread_attr_t attr;

	// Declare threads and related variables
	threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));
	
	CAPI_return_t rtnVal;
	rtnVal = CAPI_Initialize((unsigned int)numThreads);

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   float *a = (float*)malloc(SIZE*sizeof(float));
   float *b = (float*)malloc(SIZE*sizeof(float));
   float *c = (float*)malloc(SIZE*sizeof(float));

   for(unsigned int i = 0; i < SIZE; i++)
      a[i] = (float)i;

   for(unsigned int i = 0; i < SIZE; i++)
      b[i] = (float)2*i;

	for(unsigned int i = 0; i < numThreads; i++) {
      int tid = i;

      printf("Spawning worker thread: %d\n", i);
	   int err = pthread_create(&threads[i], &attr, worker, (void *) i);
	   if (err != 0) {
	      printf("  ERROR spawning thread %d!  Error code: %s\n", i, strerror(err));
	   }
	   else { 
          CAPI_message_send_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)a, sizeof(float) * SIZE);
          CAPI_message_send_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)b, sizeof(float) * SIZE);
      }
	}

	// Wait for all threads to complete
	for(unsigned int i = 0; i < numThreads; i++) 
   {
      int tid = i;
      printf("Waiting for %d\n", tid);
      CAPI_message_receive_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)numThreads, (char *)c, sizeof(float) * SIZE);
      for(unsigned int j = 0; j < SIZE; j++)
      {
         if(c[i] != a[i] * b[i])
            printf("Error in multiplication: %f should be %f * %f = %f\n", c[i], a[i], b[i], a[i] * b[i]);
         assert(c[i] == a[i] * b[i]); 
      }
      pthread_join(threads[i], NULL);
   }


   free(threads);
   free(a);
   free(b);
   free(c);

   carbonFinish();

	pthread_exit(NULL);
} // main ends

void* worker(void *threadid){

	// Declare local variables
	int tid;

	CAPI_return_t rtnVal;
	rtnVal = CAPI_Initialize((unsigned int)threadid);
	CAPI_rank(&tid);

   printf("Worker thread got id: %d\n", tid);

   float *a = (float*)malloc(SIZE*sizeof(float));
   float *b = (float*)malloc(SIZE*sizeof(float));
   float *c = (float*)malloc(SIZE*sizeof(float));
   CAPI_message_receive_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)a, sizeof(float) * SIZE);
   CAPI_message_receive_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)b, sizeof(float) * SIZE);

   for(unsigned int i = 0; i < SIZE; i++)
      c[i] = a[i] * b[i];

   printf(" Sending c...\n");
   CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)numThreads, (char *)c, sizeof(float) * SIZE);

   free(a);
   free(b);
   free(c);

   return NULL;
}

