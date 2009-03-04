// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include <unistd.h>
#include "simulator.h"
#include "config_file.hpp"
#include "thread_spawner.h"
#include "core_manager.h"

void* thread_func(void *threadid);
void* thread_func_simple(void *threadid);

void do_a_sim()
{
   config::ConfigFile cfg;

   cfg.Load("carbon_sim.cfg");
   Simulator::setConfig(&cfg);

   fprintf(stderr, "Allocating a simulator.\n");
   Simulator::allocate();
   fprintf(stderr, "Starting the simulator.\n");
   Sim()->start();


   // First start the thread spawner
   fprintf(stderr, "Spawning the thread spawner.\n");
   CarbonSpawnThreadSpawner();


   // Now bind to a core 
   fprintf(stderr, "Initializing thread.\n");
   Sim()->getCoreManager()->initializeThread();

   // Grab a comm id
   fprintf(stderr, "Initializing comm id.\n");
   Sim()->getCoreManager()->initializeCommId(3);

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

   int junk;
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

   fprintf(stderr, "Releasing the simulator.\n");
   Simulator::release();
   fprintf(stderr, "done.\n");
}

int main(int argc, char **argv)
{
        do_a_sim();

    return 0;
}

void* thread_func(void *threadid)
{
   fprintf(stderr, "Spawned  a thread func.\n");

//   Sim()->getCoreManager()->initializeThread();
   Sim()->getCoreManager()->initializeCommId((int)threadid);

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
