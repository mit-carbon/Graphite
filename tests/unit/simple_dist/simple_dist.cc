/****************************************************
 * This is a test that will test mutexes            *
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "carbon_user.h"

void* thread_func(void * threadid);

void go()
{
   const unsigned int num_threads = 2;
   carbon_thread_t threads[num_threads];

   for(unsigned int i = 0; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(thread_func, (void *) i);

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);
}

void* thread_func(void *threadid)
{
   int tid = (int)threadid;

   printf("tid: %d started\n", tid);
   return NULL;
}


int main(int argc, char* argv[])
{
   CarbonStartSim();

   for (int i = 0; i < 100; i++)
   {
      printf("Iteration %d\n", i);
      go();
   }

   CarbonStopSim();

   return 0;
}
