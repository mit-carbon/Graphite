#include "tile.h"
#include "mem_component.h"
#include "tile_manager.h"
#include "simulator.h"
#include "config.h"

#include "carbon_user.h"
#include "fixed_types.h"

using namespace std;

int main (int argc, char *argv[])
{
   printf("Starting (shared_mem_basic)\n");
   CarbonStartSim(argc, argv);

   UInt32 address = 0x1000;

   // 1) Get a tile object
   Tile* tile = Sim()->getTileManager()->getTileFromID(0);
   for (UInt32 i = 0; i < Config::getSingleton()->getTotalTiles(); i++)
   {
      Sim()->getTileManager()->getTileFromID(i)->enablePerformanceModels();
   }

   UInt32 written_val = 100;
   UInt32 read_val = 0;

   printf("Writing(%u) into address(0x%x)\n", written_val, address);
   UInt32 num_misses;
   // Write some value into this address
   num_misses = (tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::WRITE, address, (Byte*) &written_val, sizeof(written_val))).first;
   assert(num_misses == 1);
   printf("Writing(%u) into address(0x%x) completed\n", written_val, address);

   // Read out the value
   printf("Reading from address(0x%x)\n", address);
   num_misses = (tile->initiateMemoryAccess(MemComponent::L1_DCACHE, Tile::NONE, Tile::READ, address, (Byte*) &read_val, sizeof(read_val))).first;
   printf("Reading(%u) from address(0x%x) completed\n", read_val, address);
   assert(num_misses == 0);
   assert(read_val == 100);

   printf("Finished (shared_mem_basic) - SUCCESS\n");
   return 0;
}
