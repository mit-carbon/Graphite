#include "tile.h"
#include "core.h"
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
   // Enable Performance Models
   Simulator::enablePerformanceModelsInCurrentProcess();

   IntPtr address = (IntPtr) 0x1000;

   // 1) Get a tile object
   Core* core_0 = Sim()->getTileManager()->getTileFromID(0)->getCore();
   Core* core_1 = Sim()->getTileManager()->getTileFromID(1)->getCore();

   UInt32 write_val_0 = 100;
   UInt32 read_val_0 = 0;

   // Tile 0 - Write value into this address
   printf("Writing(%u) into address(%#lx)\n", write_val_0, address);
   UInt32 num_misses;
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_0, sizeof(write_val_0), true)).first;
   // assert(num_misses == 1);

   // Tile 0 - Read out the value
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0), true)).first;
   printf("Read Out(%u) from address(%#lx)\n", read_val_0, address);
   // assert(num_misses == 0);
   assert(read_val_0 == 100);

   UInt32 write_val_1 = 0;
   UInt32 read_val_1 = 0;

   // Tile 1 - Read out the value and write something else
   num_misses = (core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_1, sizeof(read_val_1), true)).first;
   printf("Read Out(%u) from address(%#lx)\n", read_val_1, address);
   // assert(num_misses == 1);
   assert(read_val_1 == 100);

   write_val_1 = read_val_1 + 10;
   // Tile 1 - Write read out value + 1
   printf("Writing(%u) into address(%#lx)\n", write_val_1, address);
   num_misses = (core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_1, sizeof(write_val_1), true)).first;
   // assert(num_misses == 1);
   
   // Tile 0 - Read out the value
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0), true)).first;
   printf("Read Out(%u) from address(%#lx)\n", read_val_0, address);
   // LOG_ASSERT_ERROR(num_misses == 1, "num misses(%i)", num_misses);
   assert(read_val_0 == 110);

   // Disable Performance Models
   Simulator::disablePerformanceModelsInCurrentProcess();
   CarbonStopSim();
   
   printf("Finished (shared_mem_test1) - SUCCESS\n");
   return 0;
}
