#include "tile.h"
#include "core.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

int num_addresses = 100;
int num_threads = 64;
int num_iterations = 10;

carbon_barrier_t barrier;
IntPtr* address;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test3)\n");
   // Start simulator
   CarbonStartSim(argc, argv);
   // Enable Models
   Simulator::enablePerformanceModelsInCurrentProcess();

   carbon_thread_t tid_list[num_threads];

   // Init barrier
   CarbonBarrierInit(&barrier, num_threads);

   Core* core = Sim()->getTileManager()->getCurrentCore();

   address = new IntPtr[num_addresses];

   for (int j = 0; j < num_addresses; j++)
   {
      int val = 0;
      address[j] = j << 18;
      printf("[MAIN] Writing (%i) into address (%#lx)\n", val, address[j]);
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address[j], (Byte*) &val, sizeof(val));
      printf("[MAIN] Writing (%i) into address (%#lx) completed\n", val, address[j]);
   }

   for (int i = 0; i < num_threads-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }
   thread_func(NULL);

   for (int i = 0; i < num_threads-1; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
  
   for (int j = 0; j < num_addresses; j++)
   {
      int val;
      printf("[MAIN] Reading from address (%#lx)\n", address[j]);
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[j], (Byte*) &val, sizeof(val));
      printf("[MAIN] Read (%i) from address (%#lx)\n", val, address[j]);
      
      if (val != (num_threads * num_iterations))
      {
         fprintf(stderr, "shared_mem_test3 (FAILURE): Address(%#lx), Expected(%i), Got(%i)\n",
                 address[j], num_threads * num_iterations, val);
         exit(-1);
      }
   }

   delete [] address;

   printf("shared_mem_test3 (SUCCESS)\n");
   
   // Disable performance models
   Simulator::disablePerformanceModelsInCurrentProcess();
   // Shutdown simulator
   CarbonStopSim();

   return 0;
}

void* thread_func(void*)
{
   CarbonBarrierWait(&barrier);
   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();

   for (int i = 0; i < num_iterations; i++)
   {
      for (int j = 0; j < num_addresses; j++)
      {
         int val;
         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::LOCK, Core::READ_EX, address[j], (Byte*) &val, sizeof(val));
         
         val += 1;

         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::UNLOCK, Core::WRITE, address[j], (Byte*) &val, sizeof(val));
      }
   }
}
