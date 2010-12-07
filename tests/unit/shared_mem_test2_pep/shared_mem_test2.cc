/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_test2.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                The main thread on main core 0 will spawn 100 new threads on main cores, as well as 100
//                helper threads.  Each thread overwrites 0x1000 by its value += num_iterations.  The main 
//                core 0 will make sure that after joining, the value of 200*num_iterations is in the memory 
//                location.
//                
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);
void* pep_thread_func(void*);

IntPtr address = 0x1000;
int num_threads = 100;
int num_iterations = 100;

int main (int argc, char *argv[])
{
   CarbonStartSim(argc, argv);
   printf("Starting (shared_mem_test2)\n");

   carbon_thread_t tid_list[num_threads];

   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   int val = 0;
   tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &val, sizeof(val));

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
   
   printf("All main threads finished! \n");

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnHelperThread(pep_thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinHelperThread(tid_list[i]);
   }

   printf("All helper threads finished! \n");
   
   tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &val, sizeof(val));
   
   printf("val(%i)\n", val);
   if (val == (2 * num_threads * num_iterations))
   {
      printf("shared_mem_test2 (SUCCESS)\n");
   }
   else
   {
      printf("shared_mem_test2 (FAILURE)\n");
   }
   
   CarbonStopSim();
   return 0;
}

void* thread_func(void*)
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::LOCK, Core::READ_EX, address, (Byte*) &val, sizeof(val));
      
      val += 1;

      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::UNLOCK, Core::WRITE, address, (Byte*) &val, sizeof(val));
   }
}

void* pep_thread_func(void*)
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::LOCK, Core::READ_EX, address, (Byte*) &val, sizeof(val));
      
      val += 1;

      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::UNLOCK, Core::WRITE, address, (Byte*) &val, sizeof(val));
   }
}
