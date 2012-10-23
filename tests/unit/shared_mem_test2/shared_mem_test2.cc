#include "tile.h"
#include "core.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

IntPtr address = 0x1000;
int num_threads = 64;
int num_iterations = 100;

carbon_barrier_t barrier;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test2)\n");
   // Start simulator
   CarbonStartSim(argc, argv);
   // Enable performance models
   Simulator::enablePerformanceModelsInCurrentProcess();

   // Init barrier
   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads];

   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();
   int val = 0;
   printf("[MAIN] Writing (%i) into address (%#lx)\n", val, address);
   core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val));
   printf("[MAIN] Writing into address (%#lx) completed\n", address);

   for (int i = 0; i < num_threads-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, NULL);
   }
   thread_func(NULL);

   for (int i = 0; i < num_threads-1; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
   
   printf("[MAIN] Reading from address (%#lx)\n", address);
   core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val));
   printf("[MAIN] Reading (%i) from address (%#lx) completed\n", val, address);
   
   if (val == (num_threads * num_iterations))
   {
      printf("shared_mem_test2 (SUCCESS)\n");
   }
   else
   {
      fprintf(stderr, "shared_mem_test2 (FAILURE): Expected(%u), Got(%i)\n", num_threads * num_iterations, val);
      exit(-1);
   }
  
   // Disable performance models
   Simulator::disablePerformanceModelsInCurrentProcess();
   // Shutdown simulator 
   CarbonStopSim();

   return 0;
}

void* thread_func(void*)
{
   // Wait on barrier
   CarbonBarrierWait(&barrier);

   Core* core = Sim()->getTileManager()->getCurrentCore();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::LOCK, Core::READ_EX, address, (Byte*) &val, sizeof(val));
      
      val += 1;

      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::UNLOCK, Core::WRITE, address, (Byte*) &val, sizeof(val));
   }
}
