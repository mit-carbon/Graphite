#include <cassert>
#include <stdio.h>

#include "carbon_user.h"
#include "fixed_types.h"
#include "core.h"
#include "memory_manager.h"
#include "simulator.h"

using namespace std;

void* threadFunc(void* thread_num);
void validateCacheModelCounters(void);
void validateDramModelCounters(void);

const SInt32 num_threads = 9;
const SInt32 shared_var = num_threads;
const SInt32 num_consecutive_reads = 2;
Core* core_list[num_threads];

carbon_barrier_t thread_barrier;

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   // 1) Create 10 threads
   // 2) Read and Write from a shared variable

   int threads[num_threads];

   CarbonBarrierInit(&thread_barrier, num_threads);

   for (SInt32 j = 0; j < num_threads; j++)
      threads[j] = CarbonSpawnThread(threadFunc, (void*) j);

   for (SInt32 j = 0; j < num_threads; j++)
      CarbonJoinThread(threads[j]);

   // Check for the correctness of evictions, misses etc
   validateCacheModelCounters();
   // Check for the correctness of dram accesses
   validateDramModelCounters();

   CarbonStopSim();
   printf("Limited Directories test successful\n");
   return 0;
}

void* threadFunc(void* threadid_ptr)
{
   SInt32 threadid = (SInt32) threadid_ptr;
   printf("Starting Thread(%i)\n", threadid);

   for (SInt32 i = 0; i < num_threads; i++)
   {
      if (threadid == i)
      {
         Core* core = Sim()->getCoreManager()->getCurrentCore();
         core_list[threadid] = core;

         if (threadid % (num_consecutive_reads+1) == 0)
         {
            // Write a shared variable
            core->accessMemory(Core::NONE, WRITE, (IntPtr) &shared_var, 
                  (char*) &threadid, sizeof(threadid), true);
         }
         else
         {
            // Read the shared variable
            SInt32 act_value;
            core->accessMemory(Core::NONE, READ, (IntPtr) &shared_var,
                  (char*) &act_value, sizeof(act_value), true);
            printf("Thread(%i), Shared Var: %i\n", threadid, act_value);

            SInt32 expected_value;
            expected_value = (threadid/(num_consecutive_reads+1)) * (num_consecutive_reads+1);
            assert(act_value == expected_value);
         }
      }
      CarbonBarrierWait(&thread_barrier);
   }

   return NULL;
}

void validateCacheModelCounters()
{
   // Get the max number of sharers
   SInt32 max_sharers;
   try
   {
      max_sharers = Sim()->getCfg()->getInt("dram_dir/max_sharers");
   }
   catch(...)
   {
      fprintf(stderr, "Cannot read 'dram_dir/max_sharers' from the config file");
   }

   printf("max_sharers(%i), num_threads(%i)\n", max_sharers, num_threads);

   for (SInt32 i = 0; i < num_threads; i++)
   {
      // This is a cached core_list
      Core* core = core_list[i];

      SInt32 buffer;
      bool is_miss = (bool) core->accessMemory(Core::NONE,
            READ,
            (IntPtr) &shared_var,
            (char*) &buffer,
            sizeof(buffer),
            false);
 
      printf("Thread(%i), is_miss: %s\n", i, (is_miss) == true ? "true" : "false");
      if (i >= (num_threads-3))
      {
         if (max_sharers >= num_threads)
            assert(is_miss == false);
      }
      else
      {
         assert(is_miss == true);
      }
   }
}

void validateDramModelCounters()
{
   UInt32 total_cores = Sim()->getConfig()->getTotalCores();

   for (SInt32 i = 0; i < (SInt32) total_cores; i++)
   {
      Core* core = Sim()->getCoreManager()->getCoreFromID(i);
      UInt64 total_dram_accesses = core->getMemoryManager()->getDramDirectory()->getDramPerformanceModel()->getTotalAccesses();

      printf("Core(%i), total_dram_accesses: %llu\n", i, total_dram_accesses);
   }
}
