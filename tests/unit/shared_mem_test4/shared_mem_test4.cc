#include "tile.h"
#include "core.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

int num_threads = 64;
int num_iterations = 100;

carbon_barrier_t barrier;

IntPtr address = 0x1000;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test4)\n");
   CarbonStartSim(argc, argv);
   Simulator::enablePerformanceModelsInCurrentProcess();

   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads];

   Core* core = Sim()->getTileManager()->getCurrentCore();

   for (int i = 0; i < num_threads-1; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, NULL);
   }
   thread_func(NULL);

   for (int i = 0; i < num_threads-1; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
  
   printf("shared_mem_test4 (SUCCESS)\n");
  
   Simulator::disablePerformanceModelsInCurrentProcess();
   CarbonStopSim();
   return 0;
}

void* thread_func(void*)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();

   for (int i = 0; i < num_iterations; i++)
   {
      if (core->getTile()->getId() == 0)
      {
         pair<UInt32,Time> ret = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &i, sizeof(i));
         LOG_PRINT("Tile(%i): Access Time(%llu ns)", core->getTile()->getId(), ret.second.toNanosec());
      }
      
      CarbonBarrierWait(&barrier);

      int val;
      pair<UInt32,Time> ret = core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val));
      LOG_PRINT("Core(%i): Access Time(%llu ns)", core->getTile()->getId(), ret.second.toNanosec());
      if (val != i)
      {
         fprintf(stderr, "shared_mem_test4 (FAILURE): Core(%i), Expected(%i), Got(%i)\n",
                 core->getTile()->getId(), i, val);
         exit(-1);
      }

      CarbonBarrierWait(&barrier);      
   }
   return NULL;
}
