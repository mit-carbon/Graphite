// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/syscall.h>
#include "carbon_user.h"

void* thread_func(void *threadid);

void do_a_sim()
{

   const unsigned int total_cores = 10;
   const unsigned int numThreads = 3;
   const unsigned int totalThreads = numThreads * total_cores/2;
   int threads[totalThreads];
   unsigned int tid;

   // Spawn 5 threads each on odd number cores.
   unsigned int l = 0;
   for(unsigned int i = 1; i < total_cores; i += 2)
   {
      for(unsigned int j = 0; j < numThreads; j++)
      {
         printf("(%5d) starting thread %i on core %i\n", syscall(__NR_gettid), j+1, i);
         tid = CarbonSpawnThreadOnTile(i, thread_func, (void *) (j + 1));
         threads[l++] = tid;

      }
   }

   for(unsigned int k = 0; k < totalThreads; k++)
   {
      printf("(%i) joining thread %i\n", syscall(__NR_gettid), k+1);
      CarbonJoinThread(threads[k]);
      printf("(%i) done joining thread %i\n", syscall(__NR_gettid), k+1);
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
   usleep(1000);
   printf("(%5d) done...\n", syscall(__NR_gettid));
   return 0;
}

