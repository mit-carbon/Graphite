#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

int num_threads = 4;
int num_iterations = 1;

carbon_barrier_t barrier;

IntPtr address = 0x1000;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test4)\n");
   CarbonStartSim(argc, argv);
   Simulator::enablePerformanceModelsInCurrentProcess();

   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads];

   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();

   int val = 0;
   core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val), true);
   LOG_PRINT("Tile(%i): Access Time(%llu)", core->getTile()->getId(), core->getShmemPerfModel()->getCycleCount());

   for (int i = 0; i < num_threads-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, NULL);
   }
   thread_func(NULL);

   for (int i = 0; i < num_threads-1; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
  
   core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val), true);
   LOG_PRINT("Tile(%i): Access Time(%llu)", core->getTile()->getId(), core->getShmemPerfModel()->getCycleCount());
   
   printf("val = %i\n", val);
   if (val != (num_iterations))
   {
      printf("shared_mem_test4 (FAILURE)\n");
   }
   else
   {
      printf("shared_mem_test4 (SUCCESS)\n");
   }
  
   Simulator::disablePerformanceModelsInCurrentProcess();
   CarbonStopSim();
   return 0;
}

void* thread_func(void*)
{
   Core* core = Sim()->getTileManager()->getCurrentTile()->getCore();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val), true);
      LOG_PRINT("Tile(%i): Access Time(%llu)", core->getTile()->getId(), core->getShmemPerfModel()->getCycleCount());

      CarbonBarrierWait(&barrier);

      if (core->getTile()->getId() == 0)
      {
         val ++;
         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val), true);
         LOG_PRINT("Tile(%i): Access Time(%llu)", core->getTile()->getId(), core->getShmemPerfModel()->getCycleCount());
      }
      
      CarbonBarrierWait(&barrier);
   }
   return NULL;
}
