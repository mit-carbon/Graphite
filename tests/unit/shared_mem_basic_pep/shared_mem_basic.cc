/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_basic.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                A main thread is spawned and writes and reads a location, it then spawns a helper thread
//                that reads the same location.  The PEP L1 cache should read the same data and only need to
//                go to the L2 cache.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"
#include "config.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

struct thread_data_t{
   UInt32 address;
};

void* main_memory_access(void *args);
void* pep_memory_access(void *args);

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_basic)\n");
   CarbonStartSim(argc, argv);

   UInt32 address = 0x1000;

   for (UInt32 i = 0; i < Config::getSingleton()->getTotalTiles(); i++)
   {
      Sim()->getTileManager()->getTileFromID(i)->enablePerformanceModels();
   }

   thread_data_t thread_args = (thread_data_t) {address};

   int tid = CarbonSpawnThread(main_memory_access, (void *) &thread_args);
   CarbonJoinThread(tid);

   return 0;
}

void* main_memory_access(void *thread_args)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   thread_data_t* args = (struct thread_data_t *) thread_args;
   UInt32 written_val = 100;
   UInt32 read_val = 0;

   printf("Main writing(%u) into args->address(0x%x)\n", written_val, args->address);
   UInt32 num_misses;
   // Write some value into this args->address
   num_misses = (core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, args->address, (Byte*) &written_val, sizeof(written_val))).first;
   assert(num_misses == 1);
   printf("Main writing(%u) into args->address(0x%x) completed\n", written_val, args->address);

   // Start a thread on the PEP core and test the current state of the L2 cache.
   thread_data_t pep_thread_args = (thread_data_t) {args->address};
   int tid = CarbonSpawnHelperThread(pep_memory_access, (void *) &pep_thread_args);
   //CarbonJoinHelperThread(tid);
   
   // Read out the value
   printf("Main reading from args->address(0x%x)\n", args->address);
   num_misses = (core->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, args->address, (Byte*) &read_val, sizeof(read_val))).first;
   printf("Main reading(%u) from args->address(0x%x) completed\n", read_val, args->address);
   assert(num_misses == 0);
   assert(read_val == 100);

   //Start a thread on the PEP core and test the current state of the L2 cache.
   //thread_data_t pep_thread_args = (thread_data_t) {args->address};
   //int tid = CarbonSpawnHelperThread(pep_memory_access, (void *) &pep_thread_args);
   //CarbonJoinHelperThread(tid);

   printf("Finished (shared_mem_basic) - SUCCESS\n");
}

void* pep_memory_access(void *thread_args)
{
   Core* core = Sim()->getTileManager()->getCurrentCore();
   thread_data_t* args = (struct thread_data_t *) thread_args;
   UInt32 read_val = 0;
   UInt32 num_misses;

   // Read out the value
   printf("PEP reading from args->address(0x%x)\n", args->address);
   num_misses = (core->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, args->address, (Byte*) &read_val, sizeof(read_val))).first;
   printf("PEP reading(%u) from args->address(0x%x) completed\n", read_val, args->address);
   assert(num_misses == 1);
   assert(read_val == 100);
}
