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

int main(int argc, char* argv[])  // main begins
{
   const unsigned int num_threads = 2;
   carbon_thread_t threads[num_threads];

   for(unsigned int i = 0; i < num_threads; i++)
   {
       threads[i] = CarbonSpawnThread(thread_func, (void *) i);

       //FIXME: why is this stupid thing needed?
       sleep(1);
   }

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   printf("Finished running simple test (dist)!.\n");

   return 0;
} // main ends

void* thread_func(void *threadid)
{
   int junk = 0xDEADBEEF;
   int junk_recv;

   // Declare local variables
   int tid = (int)threadid;

   printf("\ntid: %d started\n", tid);
   return NULL;

   CAPI_return_t rtnVal;
   rtnVal = CAPI_Initialize((int)threadid);

   sleep(10);

   printf("UserThread(%d): Sending...\n", tid);
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int));
   printf("UserThread(%d): Sent, waiting for reply...\n", tid);
   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk_recv, sizeof(int));

   if(junk_recv != junk)
       printf("UserThread(%d): Error: recv was not what was sent.\n", tid);
   else
       printf("UserThread(%d): Success: recv was what was sent.\n", tid);

   printf("UserThread(%d): Done!\n", tid);
}


