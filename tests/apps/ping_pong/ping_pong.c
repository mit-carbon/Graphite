#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "carbon_user.h"

void* ping_pong(void *threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);

   int num_threads = 2;
   carbon_thread_t threads[num_threads];

   for(unsigned int i = 0; i < num_threads; i++)
   {
       printf("Spawning thread: %d\n", i);
       threads[i] = CarbonSpawnThread(ping_pong, (void *) i);
   }

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   printf("Finished running PingPong!.\n");

   CarbonStopSim();
   return 0;
} // main ends


void* ping_pong(void *threadid)
{
   int junk;
   int tid = (int)threadid;
   printf("Thread: %d spawned!\n", tid);

   CAPI_Initialize((int)threadid);

   //FIXME: there is a race condition on claiming a comm id...
   
   sleep(5);

   printf("sending.\n");
   CAPI_message_send_w((CAPI_endpoint_t) tid, !tid, (char*) &junk, sizeof(int));
   CAPI_message_receive_w((CAPI_endpoint_t) !tid, tid, (char*) &junk, sizeof(int));
   

   return NULL;
}

