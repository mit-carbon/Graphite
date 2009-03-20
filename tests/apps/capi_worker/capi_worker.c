/******************************************************************
 * This test sends arrays values to a sub-thread to be multiplied.*
 *                      - Charles Gruenwald  (12/12/2008)         *
 *****************************************************************/
#include <assert.h>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "capi.h"
#include "fixed_types.h"

#define SIZE 8

void* worker(void * threadid);

unsigned int numThreads = 2;

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim();

   pthread_t* threads;
   pthread_attr_t attr;
   float m[SIZE];
   int c = 3;

   for (unsigned int i = 0; i < SIZE; i++)
      m[i] = (float)i;

   CAPI_Initialize((unsigned int)numThreads);

   threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   for (unsigned int i = 0; i < numThreads; i++)
   {
      int tid = i;

      printf("Spawning worker thread: %d\n", i);
      pthread_create(&threads[i], &attr, worker, (void *) i);
      Boolean started;
      CAPI_message_receive_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)numThreads, (char *)&started, sizeof(started));

      CAPI_message_send_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)&c, sizeof(c));
      CAPI_message_send_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)tid, (char *)m, sizeof(float) * SIZE);
   }

   printf("Done... joining.\n");
   for (unsigned int i = 0; i < numThreads; i++)
      pthread_join(threads[i], NULL);

   free(threads);
   pthread_exit(NULL);
   printf("Done\n");

   CarbonStopSim();
} // main ends

void* worker(void *threadid)
{
   int c;
   float m[SIZE];
   Boolean started = 1;

   CAPI_Initialize((unsigned int)threadid);

   CAPI_message_send_w((CAPI_endpoint_t)threadid, (CAPI_endpoint_t)numThreads, (char *)&started, sizeof(started));

   CAPI_message_receive_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)threadid, (char *)&c, sizeof(c));
   CAPI_message_receive_w((CAPI_endpoint_t)numThreads, (CAPI_endpoint_t)threadid, (char *)m, sizeof(float) * SIZE);

   pthread_exit(NULL);
}

