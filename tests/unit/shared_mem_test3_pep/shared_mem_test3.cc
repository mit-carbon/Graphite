/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_test3.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                There are 100 separate addresses at (1 to 100) << 18.
//
//                The main thread on main core 0 will spawn 125 new threads on main cores, as well as 125
//                helper threads.  Each thread overwrites all 100 addresses by value += num_iterations.   
//                Core 0 will make sure that after joining, the value of 200*num_iterations is in every memory 
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

int num_addresses = 100;
int num_threads = 125;
int num_iterations = 100;

IntPtr* address;

int main (int argc, char *argv[])
{
   CarbonStartSim(argc, argv);
   printf("Starting (shared_mem_test3)\n");

   carbon_thread_t tid_list[num_threads];

   Tile* tile = Sim()->getTileManager()->getCurrentTile();
   bool is_fail = false;

   address = new IntPtr[num_addresses];

   for (int j = 0; j < num_addresses; j++)
   {
      int val = 0;
      address[j] = j << 18;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address[j], (Byte*) &val, sizeof(val));
   }

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinThread(tid_list[i]);
   }
 
   printf("Main threads finished!");

   for (int i = 0; i < num_threads; i++)
   {
      tid_list[i] = CarbonSpawnHelperThread(pep_thread_func, (void*) i);
   }

   for (int i = 0; i < num_threads; i++)
   {
      CarbonJoinHelperThread(tid_list[i]);
   }
   
   printf("Helper threads finished!");

   for (int j = 0; j < num_addresses; j++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[j], (Byte*) &val, sizeof(val));
      
      printf("val[%i] = %i\n", j, val);
      if (val != (2 * num_threads * num_iterations))
      {
         printf("shared_mem_test3 (FAILURE)\n");
         is_fail = true;
      }
   }

   if (!is_fail)
      printf("shared_mem_test3 (SUCCESS)\n");
  
   delete [] address;

   CarbonStopSim();
   return 0;
}

void* thread_func(void*)
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      for (int j = 0; j < num_addresses; j++)
      {
         int val;
         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::LOCK, Core::READ_EX, address[j], (Byte*) &val, sizeof(val));
         
         val += 1;

         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::UNLOCK, Core::WRITE, address[j], (Byte*) &val, sizeof(val));
      }
   }
}

void* pep_thread_func(void*)
{
   Tile* tile = Sim()->getTileManager()->getCurrentTile();

   for (int i = 0; i < num_iterations; i++)
   {
      for (int j = 0; j < num_addresses; j++)
      {
         int val;
         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::LOCK, Core::READ_EX, address[j], (Byte*) &val, sizeof(val));
         
         val += 1;

         tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::UNLOCK, Core::WRITE, address[j], (Byte*) &val, sizeof(val));
      }
   }
}
