/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_test1.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                Main core 0 writes to a memory location, and then reads out the cached value.  Main core
//                1 will then overwrite it.  Main core 0 will read the new written value.
//
//                PEP core 0 will read the cached value.  PEP core 1 will overwrite it.  PEP core 0 will
//                read the new written value.
//
//                Main core 0 will then read it from its L1.
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_test1)\n");
   CarbonStartSim(argc, argv);

   UInt32 address = 0x1000;

   // Enable Performance Models
   for (UInt32 i = 0; i < Config::getSingleton()->getTotalTiles(); i++)
      Sim()->getTileManager()->getTileFromID(i)->enablePerformanceModels();

   // 1) Get a tile object
   Core* main_core_0 = Sim()->getTileManager()->getCoreFromID((core_id_t) {0, 0});
   Core* main_core_1 = Sim()->getTileManager()->getCoreFromID((core_id_t) {1, 0});
   Core* pep_core_0 = Sim()->getTileManager()->getCoreFromID((core_id_t) {0, 1});
   Core* pep_core_1 = Sim()->getTileManager()->getCoreFromID((core_id_t) {1, 1});

   UInt32 write_val_0 = 100;
   UInt32 read_val_0 = 0;

   // ************** Main Core Action *************************
   
   // Main Core 0 - Write value into this address
   printf("Main Core 0 Writing(%u) into address(0x%x)\n", write_val_0, address);
   UInt32 num_misses;
   num_misses = (main_core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_0, sizeof(write_val_0))).first;
   assert(num_misses == 1);

   // Main Core 0 - Read out the value
   num_misses = (main_core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("Main Core 0 Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 0);
   assert(read_val_0 == 100);

   UInt32 write_val_1 = 0;
   UInt32 read_val_1 = 0;

   // Main Core 1 - Read out the value and write something else
   num_misses = (main_core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_1, sizeof(read_val_1))).first;
   printf("Main Core 1 Read Out(%u) from address(0x%x)\n", read_val_1, address);
   assert(num_misses == 1);
   assert(read_val_1 == 100);

   write_val_1 = read_val_1 + 10;
   // Main Core 1 - Write read out value + 1
   printf("Main Core 1 Writing(%u) into address(0x%x)\n", write_val_1, address);
   num_misses = (main_core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_1, sizeof(write_val_1))).first;
   assert(num_misses == 1);
   
   // Main Core 0 - Read out the value
   num_misses = (main_core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("Main Core 1 Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 1);
   assert(read_val_0 == 110);

   
   // ************** PEP Core Action *************************
   
   // PEP Core 0 - Read out the value
   num_misses = (pep_core_0->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("PEP Core 0 Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 1);
   assert(read_val_0 == 110);

   // PEP Core 1 - Read out the value and write something else
   num_misses = (pep_core_1->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_1, sizeof(read_val_1))).first;
   printf("PEP Core 1 Read Out(%u) from address(0x%x)\n", read_val_1, address);
   assert(num_misses == 1);
   assert(read_val_1 == 110);

   write_val_1 = read_val_1 + 10;
   // PEP Core 1 - Write read out value + 1
   printf("PEP Core 1 Writing(%u) into address(0x%x)\n", write_val_1, address);
   num_misses = (pep_core_1->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_1, sizeof(write_val_1))).first;
   assert(num_misses == 1);
   
   // PEP Core 0 - Read out the value
   num_misses = (pep_core_0->initiateMemoryAccess(MemComponent::L1_PEP_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("PEP Core 0 Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 1);
   assert(read_val_0 == 120);

   // Main Core 0 - Read out the value
   num_misses = (main_core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("Main Core 0 Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 1);
   assert(read_val_0 == 120);

   // Disable Performance Models
   for (UInt32 i = 0; i < Config::getSingleton()->getTotalTiles(); i++)
      Sim()->getTileManager()->getTileFromID(i)->disablePerformanceModels();
   
   printf("Finished (shared_mem_test1) - SUCCESS\n");
   CarbonStopSim();
   return 0;
}
