// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/syscall.h>
#include "carbon_user.h"

void* thread_func(void *threadid);

void do_a_sim()
{

   const unsigned int numThreads = 5;
   int threads[numThreads];

   for(unsigned int j = 0; j < numThreads; j++)
   {
      printf("(%5d) starting thread %i\n", syscall(__NR_gettid), j+1);
      threads[j] = CarbonSpawnThread(thread_func, (void *) (j + 1));
      printf("(%5d) done starting thread %i\n", syscall(__NR_gettid), j+1);
   }

   for(unsigned int j = 0; j < numThreads; j++)
   {
      printf("(%i) joining thread %i\n", syscall(__NR_gettid), j+1);
       CarbonJoinThread(threads[j]);
      printf("(%i) done joining thread %i\n", syscall(__NR_gettid), j+1);
   }

}

int main(int argc, char **argv)
{
   int i = 0;
   CarbonStartSim(argc, argv);

   for(i = 0; i < 1; i++)
   {
       fprintf(stderr, "test %d...\n", i);
       do_a_sim();
   }
   CarbonStopSim();

   fprintf(stderr, "done.\n");
   i++;
   return 0;
}

void* thread_func(void *threadid)
{
   printf("(%5d) sleeping...\n", syscall(__NR_gettid));
   usleep(10000000);
   printf("(%5d) done...\n", syscall(__NR_gettid));
   return 0;
}

