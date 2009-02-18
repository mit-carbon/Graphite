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
   int proc = atoi(argv[1]);
   fprintf(stderr, "Process: %d\n", proc);


   thread_func(!proc);
} // main ends

void* thread_func(void *threadid)
{
   int junk = 0xDEADBEEF;
   int junk_recv;

   // Declare local variables
   int tid = (int)threadid;
   CAPI_return_t rtnVal;

   carbonInitializeThread();
   rtnVal = CAPI_Initialize((int)threadid);

   sleep(1);

   // Initialize local variables
//   CAPI_rank(&tid);

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


