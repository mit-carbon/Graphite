#include "tile.h"
#include "mem_component.h"
#include "core_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

int num_threads = 2;
int num_iterations = 1;

carbon_barrier_t barrier;

IntPtr address = 0x1000;

int main (int argc, char *argv[])
{
   CarbonStartSim(argc, argv);
   printf("Starting (shared_mem_test4)\n");

   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads];

   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   int val = 0;
   tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::WRITE, address, (Byte*) &val, sizeof(val));
   LOG_PRINT("Tile(%i): Access Time(%llu)", tile->getId(), tile->getShmemPerfModel()->getCycleCount());

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
  
   tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::READ, address, (Byte*) &val, sizeof(val));
   LOG_PRINT("Tile(%i): Access Time(%llu)", tile->getId(), tile->getShmemPerfModel()->getCycleCount());
   
   printf("val = %i\n", val);
   if (val != (num_iterations))
   {
      printf("shared_mem_test4 (FAILURE)\n");
   }
   else
   {
      printf("shared_mem_test4 (SUCCESS)\n");
   }
  
   CarbonStopSim();
   return 0;
}

void* thread_func(void* threadid)
{
   long tid = (long) threadid;
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::READ, address, (Byte*) &val, sizeof(val));
      LOG_PRINT("Tile(%i): Access Time(%llu)", tile->getId(), tile->getShmemPerfModel()->getCycleCount());

      CarbonBarrierWait(&barrier); 

      if (tid == 0)
      {
         val ++;
         tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::WRITE, address, (Byte*) &val, sizeof(val));
         LOG_PRINT("Tile(%i): Access Time(%llu)", tile->getId(), tile->getShmemPerfModel()->getCycleCount());
      }
   }
   return NULL;
}
