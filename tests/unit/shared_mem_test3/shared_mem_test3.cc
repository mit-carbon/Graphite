#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

void* thread_func(void*);

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
  
   for (int j = 0; j < num_addresses; j++)
   {
      int val;
      tile->getCurrentCore()->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address[j], (Byte*) &val, sizeof(val));
      
      printf("val[%i] = %i\n", j, val);
      if (val != (num_threads * num_iterations))
      {
         printf("shared_mem_test3 (FAILURE)\n");
      }
   }

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
