#include <stdio.h>
#include <cassert>

#include "carbon_user.h"
#include "fixed_types.h"
#include "simulator.h"
#include "tile_manager.h"
#include "core.h"
#include "memory_manager.h"

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   const UInt32 size = 10;

   IntPtr address_list[size] = {
      0x10 << 6,
      0x20 << 6,
      0x50 << 6,
      0x85 << 6,
      0x77 << 6,
      0x100 << 6,
      0x167 << 6,
      0x15 << 6,
      0x199 << 6,
      0x131 << 6
   };

   UInt32 buff[size];
   // Initialize buff
   for (UInt32 i = 0; i < size; i++)
      buff[i] = i;

   // Get the cores
   Tile* cores[2];
   for (UInt32 j = 0; j < 2; j++)
      cores[j] = Sim()->getTileManager()->getTileFromID(j);

   // Read many lines into cache
   // 10 * 2 cold misses
   for (UInt32 i = 0; i < size; i++)
   {
      shmem_req_t shmem_req_type;
      if ((i % 2) == 0)
         shmem_req_type = READ;
      else
         shmem_req_type = WRITE;

      for (UInt32 j = 0; j < 2; j++)
      {
         bool is_miss = (bool) cores[j]->accessMemory(Core::NONE, shmem_req_type, address_list[i], (char*) &buff[i], sizeof(buff[i]), true);
         assert (is_miss == true);
      }
   }

   // Invalidate some lines
   for (UInt32 i = 0; i < size; i += 3)
   {
      bool is_miss = (bool) cores[1]->accessMemory(Core::NONE, WRITE, address_list[i], (char*) &buff[i], sizeof(buff[i]), true);
      printf("Tile(1) WRITE: %i, is_miss: %s\n", i, (is_miss) ? "true" : "false");
   }

   printf ("\n\n");

   // Read some lines
   for (UInt32 i = 0; i < size; i += 4)
   {
      bool is_miss = (bool) cores[0]->accessMemory(Core::NONE, READ, address_list[i], (char*) &buff[i], sizeof(buff[i]), true);
      printf("Tile(0) READ: %i, is_miss: %s\n", i, (is_miss) ? "true" : "false");
   }

   printf ("\n\n");

   // Write some lines again
   for (UInt32 i = 0; i < size; i += 2)
   {
      bool is_miss = (bool) cores[0]->accessMemory(Core::NONE, WRITE, address_list[i], (char*) &buff[i], sizeof(buff[i]), true);
      printf("Tile(0) WRITE: %i, is_miss: %s\n", i, (is_miss) ? "true" : "false");
   }

   IntPtr address_list_2[size] = {
      0x100 << 16,
      0x200 << 16,
      0x300 << 16,
      0x400 << 16,
      0x100 << 16,
      0x500 << 16,
      0x300 << 16,
      0x100 << 16,
      0x200 << 16,
      0x400 << 16
   };

   printf ("\n\n");

   for (UInt32 i = 0; i < size; i++)
   {
      bool is_miss = (bool) cores[0]->accessMemory(Core::NONE, WRITE, address_list_2[i], (char*) &buff[i], sizeof(buff[i]), true);
      printf("Tile(0) WRITE: 0x%x, is_miss: %s\n", address_list_2[i] >> 16, (is_miss) ? "true" : "false");
   }

   CarbonStopSim();

   printf("Cache Counters test successful\n");
   
   return 0;
}
