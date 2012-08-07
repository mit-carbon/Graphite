// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/syscall.h>
#include "carbon_user.h"

void* thread_func(void *threadid);

void do_a_sim()
{

   const unsigned int total_cores = 6;
   const unsigned int numThreads = 5;
   int threads[numThreads+1];
   unsigned int tid;

   // Spawn 5 threads on core 1, then move 4 of them to cores 2-5.
   for(unsigned int j = 0; j < numThreads; j++)
   {
      printf("(%5d) starting thread %i on core %i\n", syscall(__NR_gettid), j+1, 1);
      threads[j] = CarbonSpawnThreadOnTile((tile_id_t) 1, thread_func, (void *) (j + 1));
   }


   cpu_set_t * set = CPU_ALLOC(total_cores);
   size_t alloc_size = CPU_ALLOC_SIZE(total_cores);
   unsigned int cpusetsize = total_cores;
   for(unsigned int k = 1; k < numThreads; k++)
   {
      CPU_ZERO_S(alloc_size, set);
      CPU_SET_S(k + 1, alloc_size, set);

      printf("(%5d) setting affinity of thread %i to tile %i\n", syscall(__NR_gettid), threads[k], k+1);
      CarbonSchedSetAffinity(threads[k], cpusetsize, set);
   }

   for(unsigned int k = 1; k < numThreads; k++)
   {
      CarbonSchedGetAffinity(threads[k], cpusetsize, set);

      printf("thread %i mask: ", threads[k]);
      for (unsigned int i = 0; i < cpusetsize; i++)
         if (CPU_ISSET_S(i, CPU_ALLOC_SIZE(cpusetsize), set) != 0)
            printf("%i ", i);
      printf("\n");
   }

   for(unsigned int k = 0; k < numThreads; k++)
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
   usleep(5000000);
   printf("(%5d) done...\n", syscall(__NR_gettid));
   return 0;
}

