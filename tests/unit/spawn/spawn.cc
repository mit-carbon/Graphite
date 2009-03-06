// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include "test_support.h"

void* thread_func(void *threadid);
void* thread_func_simple(void *threadid);

void do_a_sim()
{

   // Grab a comm id
   CAPI_Initialize(3);

   for(unsigned int i = 0; i < 500; i++)
   {
      int tid0 = CarbonSpawnThread(thread_func_simple, (void*)0);
      CarbonJoinThread(tid0);
   }

   // Spawn some threads
   fprintf(stderr, "Spawning child thread 0.\n");
   int tid1 = CarbonSpawnThread(thread_func, (void*)0);

   fprintf(stderr, "Spawning child thread 1.\n");
   int tid2 = CarbonSpawnThread(thread_func, (void*)1);

   // Give them a chance to start up
   usleep(500);

   unsigned int junk;
   fprintf(stderr,"Waiting for msg from 0\n");
   CAPI_message_receive_w(0, 3, (char *)&junk, sizeof(junk));
   assert(junk == 0xDEADBEEF);
   fprintf(stderr,"Waiting for msg from 1\n");
   CAPI_message_receive_w(1, 3, (char *)&junk, sizeof(junk));
   assert(junk == 0xDEADBEEF);

   fprintf(stderr,"joining 0\n");
   CarbonJoinThread(tid1);

   fprintf(stderr,"joining 1\n");
   CarbonJoinThread(tid2);
}

int main(int argc, char **argv)
{
   CarbonStartSim();
   do_a_sim();
   CarbonStopSim();

   fprintf(stderr, "done.\n");
   return 0;
}

void* thread_func(void *threadid)
{
   fprintf(stderr, "Spawned  a thread func.\n");

   CAPI_Initialize((int)threadid);

   usleep(500);

   fprintf(stderr, "Sending...\n");
   int junk = 0xDEADBEEF;
   CAPI_message_send_w((int)threadid, 3, (char *)&junk, sizeof(junk));

   fprintf(stderr, "Yeah thread\n");
   return 0;
}

void* thread_func_simple(void *threadid)
{
   return 0;
}
