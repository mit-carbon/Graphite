#include "core.h"
#include "mem_component.h"
#include "core_manager.h"
#include "simulator.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_basic)\n");
   CarbonStartSim(argc, argv);

   UInt32 address = 0x1000;

   // 1) Get a core object
   Core* core_0 = Sim()->getCoreManager()->getCoreFromID(0);
   Core* core_1 = Sim()->getCoreManager()->getCoreFromID(1);

   UInt32 write_val_0 = 100;
   UInt32 read_val_0 = 0;

   // Core 0 - Write value into this address
   printf("Writing(%u) into address(0x%x)\n", write_val_0, address);
   UInt32 num_misses;
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_0, sizeof(write_val_0))).first;
   assert(num_misses == 1);

   // Core 0 - Read out the value
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 0);
   assert(read_val_0 == 100);

   UInt32 write_val_1 = 0;
   UInt32 read_val_1 = 0;

   // Core 1 - Read out the value and write something else
   num_misses = (core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_1, sizeof(read_val_1))).first;
   printf("Read Out(%u) from address(0x%x)\n", read_val_1, address);
   assert(num_misses == 1);
   assert(read_val_1 == 100);

   write_val_1 = read_val_1 + 10;
   // Core 1 - Write read out value + 1
   printf("Writing(%u) into address(0x%x)\n", write_val_1, address);
   num_misses = (core_1->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::WRITE, address, (Byte*) &write_val_1, sizeof(write_val_1))).first;
   assert(num_misses == 1);
   
   // Core 0 - Read out the value
   num_misses = (core_0->initiateMemoryAccess(MemComponent::L1_DCACHE, Core::NONE, Core::READ, address, (Byte*) &read_val_0, sizeof(read_val_0))).first;
   printf("Read Out(%u) from address(0x%x)\n", read_val_0, address);
   assert(num_misses == 1);
   assert(read_val_0 == 110);

   CarbonStopSim();
   printf("Finished (shared_mem_basic) - SUCCESS\n");
   return 0;
}
