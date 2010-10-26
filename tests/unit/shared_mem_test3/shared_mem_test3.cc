#include "core.h"
#include "mem_component.h"
#include "core_manager.h"
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

   Tile* core = Sim()->getCoreManager()->getCurrentCore();

   address = new IntPtr[num_addresses];

   for (int j = 0; j < num_addresses; j++)
   {
      int val = 0;
      address[j] = j << 18;
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::WRITE, address[j], (Byte*) &val, sizeof(val));
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
      core->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::READ, address[j], (Byte*) &val, sizeof(val));
      
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
   Tile* core = Sim()->getCoreManager()->getCurrentCore();

   for (int i = 0; i < num_iterations; i++)
   {
      for (int j = 0; j < num_addresses; j++)
      {
         int val;
         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::LOCK, Tile::READ_EX, address[j], (Byte*) &val, sizeof(val));
         
         val += 1;

         core->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::UNLOCK, Tile::WRITE, address[j], (Byte*) &val, sizeof(val));
      }
   }
}
