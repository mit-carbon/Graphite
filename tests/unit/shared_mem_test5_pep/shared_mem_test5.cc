/////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// File: shared_mem_test5.cc
//
// Description:   This tests the memory model where the PEP and main cores have separate L1 caches, but
//                share an L2 cache.
//
//                A variety of memory operations are made, and we check if the expected cache states are
//                valid on the main and pep cores.  
//
//                **MSI protocol with FULL map cache only!!**
//
/////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "carbon_user.h"

#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "memory_manager_base.h"

#include "cache.h"
#include "cache_base.h"
#include "cache_state.h"
#include "cache_block_info.h"
#include "pr_l1_cache_block_info.h"
#include "pr_l2_cache_block_info.h"

#include "directory_block_info.h"
#include "directory_state.h"
#include "directory_entry_limited_broadcast.h"

#include "mem_component.h"
#include "address_home_lookup.h"

#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/memory_manager.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/l1_cache_cntlr.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/l2_cache_cntlr.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/dram_directory_cache.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/dram_directory_cntlr.h"
#include "pr_l1_pr_l1_pr_l2_dram_directory_msi/dram_cntlr.h"

#include "log.h"

void check(IntPtr address, DirectoryState::dstate_t dstate, tile_id_t owner, unsigned int num_sharers, int tracked_sharers_list, int cached_val, int memory_val);
bool isSharer(tile_id_t i, int tracked_sharers_list);

Tile** tile;
int* core_val;
IntPtr address_0 = (IntPtr) 0x800;
IntPtr address_1 = (IntPtr) 0x400;

enum
{
   TILE1 = 1 << 1,
   TILE2 = 1 << 2,
   TILE3 = 1 << 3
};

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   unsigned int total_cores = Sim()->getConfig()->getTotalTiles();
   tile = new Tile*[total_cores];
   core_val = new int[total_cores];

   // Let have 4 application cores for now
   for (tile_id_t i = 0; i < total_cores; i++)
   {
      tile[i] = Sim()->getTileManager()->getTileFromID(i);
      core_val[i] = i * 100;
   }

   int buf = 0;

   tile[1]->getCore()->accessMemory(Core::NONE, Core::WRITE, address_0, (char*) &core_val[1], sizeof(core_val[1]));
   // MainCore1 writes address_0
   // Assertions:
   // [[address_0]]
   // State -> MODIFIED
   // num sharers -> 1
   // Owner -> TILE1
   sleep(1);
   check(address_0, DirectoryState::MODIFIED, 1, 1, TILE1, 100, 0);
   printf("Test1 Passed\n");

   tile[2]->getCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   // MainCore2 reads address_0 
   // Assertions:
   // [[address_0]]
   // State -> OWNED
   // num sharers -> 2
   // Owner -> INVALID_TILE_ID
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE2, 100, 100);
   tile[2]->getPepCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE2, 100, 100);
   printf("Test2 Passed\n");

   tile[3]->getCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   // Core3 reads address_0
   // Assertions:
   // [[address_0]]
   // State -> SHARED
   // num sharers -> 3
   // Owner -> INVALID_TILE_ID
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 3, TILE1 + TILE2 + TILE3, 100, 100);
   tile[3]->getPepCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 3, TILE1 + TILE2 + TILE3, 100, 100);
   printf("Test3 Passed\n");

   tile[1]->getCore()->accessMemory(Core::NONE, Core::READ, address_1, (char*) &buf, sizeof(buf));
   // MainCore1 reads address_1
   // Assertions:
   // [[address_0]]
   // State -> SHARED
   // num sharers -> 2
   // Owner -> INVALID_TILE_ID
   // [[address_1]]
   // State -> SHARED
   // num sharers -> 1
   // Owner -> INVALID_TILE_ID
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 3, TILE1 + TILE2 + TILE3, 100, 100);
   check(address_1, DirectoryState::SHARED, INVALID_TILE_ID, 1, TILE1, 0, 0);
   printf("Test4 Passed\n");

   tile[2]->getCore()->accessMemory(Core::NONE, Core::WRITE, address_0, (char*) &core_val[2], sizeof(core_val[2]));
   // MainCore2 writes address_i0
   // Assertions:
   // [[address_0]]
   // State -> MODIFIED
   // num sharers -> 1
   // Owner -> Core2
   sleep(1);
   check(address_0, DirectoryState::MODIFIED, 2, 1, TILE2, 200, 100);
   tile[2]->getPepCore()->accessMemory(Core::NONE, Core::WRITE, address_0, (char*) &core_val[2], sizeof(core_val[2]));
   sleep(1);
   check(address_0, DirectoryState::MODIFIED, 2, 1, TILE2, 200, 100);
   printf("Test5 Passed\n");
   
   tile[2]->getCore()->accessMemory(Core::NONE, Core::READ, address_1, (char*) &buf, sizeof(buf));
   // MainCore2 reads address_1
   // Assertions:
   // [[address_0]]
   // State -> UNCACHED
   // num sharers -> 0
   // Owner -> INVALID_TILE_ID
   // [[address_1]]
   // State -> SHARED
   // num sharers -> 2
   // Owner -> INVALID_Tile_ID
   sleep(1);
   check(address_1, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE2, 0, 0);  
   printf("Test6 Passed\n");

   tile[3]->getPepCore()->accessMemory(Core::NONE, Core::WRITE, address_1, (char*) &core_val[3], sizeof(core_val[3]));
   // Core3 writes address_1
   // Assertions:
   // [[address_1]]
   // State -> MODIFIED
   // num sharers -> 1
   // Owner -> Core3
   sleep(1);
   check(address_1, DirectoryState::MODIFIED, 3, 1, TILE3, 300, 0);
   printf("Test7 Passed\n");

   tile[1]->getCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   // MainCore1 reads address_0
   // Assertions:
   // [[address_0]]
   // State -> SHARED
   // num sharers -> 1
   // Owner -> INVALID_TILE_ID
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE2, 200, 200);
   tile[1]->getPepCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE2, 200, 200);
   printf("Test8 Passed\n");

   tile[1]->getPepCore()->accessMemory(Core::NONE, Core::WRITE, address_0, (char*) &core_val[1], sizeof(core_val[1]));
   // PepCore1 writes address_0
   // Assertions:
   // [[address_0]]
   // State -> MODIFIED
   // num sharers -> 1
   // Owner -> Core1
   sleep(1);
   check(address_0, DirectoryState::MODIFIED, 1, 1, TILE1, 100, 200);
   printf("Test9 Passed\n");

   tile[3]->getPepCore()->accessMemory(Core::NONE, Core::READ, address_0, (char*) &buf, sizeof(buf));
   // PepCore3 reads address_0
   // Assertions:
   // [[address_0]]
   // State -> OWNED
   // num sharers -> 2
   // Owner -> TILE1
   sleep(1);
   check(address_0, DirectoryState::SHARED, INVALID_TILE_ID, 2, TILE1 + TILE3, 100, 100);
   printf("Test10 Passed\n");
   printf("All Tests Passed\n");

   CarbonStopSim();
   return 0;
}

void check(IntPtr address, DirectoryState::dstate_t dstate, tile_id_t owner, unsigned int num_sharers, int tracked_sharers_list, int cached_val, int memory_val)
{
   printf("--> Checking address(0x%x)\n", address);
   tile_id_t owner_tile = owner;

   for (tile_id_t i = 1; i <= 3; i++)
   {
      PrL1PrL1PrL2DramDirectoryMSI::MemoryManager* memory_manager = (PrL1PrL1PrL2DramDirectoryMSI::MemoryManager*) tile[i]->getMemoryManager();
      Cache* pr_main_l1_dcache = memory_manager->getMainL1DCache();
      Cache* pr_pep_l1_dcache = memory_manager->getPepL1DCache();
      Cache* pr_l2_cache = memory_manager->getL2Cache();

      CacheState::cstate_t actual_main_l1d_cstate;
      CacheState::cstate_t actual_pep_l1d_cstate;
      CacheState::cstate_t actual_l2_cstate;
      int actual_main_l1d_cached_val;
      int actual_pep_l1d_cached_val;
      int actual_l2_cached_val;
      int actual_cached_val;
      
      PrL1CacheBlockInfo* pr_main_l1_cache_block_info = (PrL1CacheBlockInfo*) pr_main_l1_dcache->accessSingleLine(address, CacheBase::LOAD, (Byte*) &actual_main_l1d_cached_val, sizeof(int));
      PrL1CacheBlockInfo* pr_pep_l1_cache_block_info = (PrL1CacheBlockInfo*) pr_pep_l1_dcache->accessSingleLine(address, CacheBase::LOAD, (Byte*) &actual_pep_l1d_cached_val, sizeof(int));
      PrL2CacheBlockInfo* pr_l2_cache_block_info = (PrL2CacheBlockInfo*) pr_l2_cache->accessSingleLine(address, CacheBase::LOAD, (Byte*) &actual_l2_cached_val, sizeof(int));

      // Check inclusivity.
      assert((pr_main_l1_cache_block_info == NULL) == (pr_l2_cache_block_info == NULL) || (pr_pep_l1_cache_block_info == NULL) == (pr_l2_cache_block_info == NULL));

      if (pr_main_l1_cache_block_info && pr_pep_l1_cache_block_info)
      {
         actual_main_l1d_cstate = pr_main_l1_cache_block_info->getCState();
         actual_pep_l1d_cstate = pr_pep_l1_cache_block_info->getCState();
         actual_l2_cstate = pr_l2_cache_block_info->getCState();

         assert(pr_l2_cache_block_info->getCachedLoc() == MemComponent::L1_DCACHE || pr_l2_cache_block_info->getCachedLoc() == MemComponent::L1_PEP_DCACHE);
         assert(actual_main_l1d_cached_val == actual_pep_l1d_cached_val);
         assert(actual_main_l1d_cached_val == actual_l2_cached_val);
         assert(actual_pep_l1d_cached_val == actual_l2_cached_val);

         actual_cached_val = actual_main_l1d_cached_val;
      }
      else if (pr_main_l1_cache_block_info)
      {
         actual_main_l1d_cstate = pr_main_l1_cache_block_info->getCState();
         actual_l2_cstate = pr_l2_cache_block_info->getCState();
         assert(pr_l2_cache_block_info->getCachedLoc() == MemComponent::L1_DCACHE);
         
         assert(actual_main_l1d_cached_val == actual_l2_cached_val);
         actual_cached_val = actual_main_l1d_cached_val;
      }
      else if (pr_pep_l1_cache_block_info)
      {
         actual_pep_l1d_cstate = pr_pep_l1_cache_block_info->getCState();
         actual_l2_cstate = pr_l2_cache_block_info->getCState();
         assert(pr_l2_cache_block_info->getCachedLoc() == MemComponent::L1_PEP_DCACHE);
         
         assert(actual_pep_l1d_cached_val == actual_l2_cached_val);
         actual_cached_val = actual_pep_l1d_cached_val;
      }
      else
      {
         actual_main_l1d_cstate = CacheState::INVALID;
         actual_pep_l1d_cstate = CacheState::INVALID;
         actual_l2_cstate = CacheState::INVALID;
         
         actual_cached_val = 0;
      }

      switch(dstate)
      {
         case DirectoryState::MODIFIED:
            if (owner_tile == i)
            {
               assert(actual_main_l1d_cstate == CacheState::MODIFIED || actual_pep_l1d_cstate == CacheState::MODIFIED);
               assert(actual_l2_cstate == CacheState::MODIFIED);
               assert(actual_cached_val == cached_val);
            }
            else
            {
               assert(actual_main_l1d_cstate == CacheState::INVALID && actual_pep_l1d_cstate == CacheState::INVALID);
               assert(actual_l2_cstate == CacheState::INVALID);
            }
            break;

         case DirectoryState::OWNED:
            assert(false);
            break;

         case DirectoryState::SHARED:
            if (isSharer(i, tracked_sharers_list))
            {
               assert(actual_main_l1d_cstate == CacheState::SHARED || actual_pep_l1d_cstate == CacheState::SHARED);
               assert(actual_l2_cstate == CacheState::SHARED);
               assert(actual_cached_val == cached_val);
            }
            else
            {
               assert((actual_main_l1d_cstate == CacheState::INVALID) || (actual_main_l1d_cstate == CacheState::SHARED));
               assert((actual_pep_l1d_cstate == CacheState::INVALID) || (actual_pep_l1d_cstate == CacheState::SHARED));
               assert((actual_l2_cstate == CacheState::INVALID) || (actual_l2_cstate == CacheState::SHARED));

               if (actual_main_l1d_cstate == CacheState::SHARED || actual_pep_l1d_cstate == CacheState::SHARED)
                  assert(actual_cached_val == cached_val);
            }
            break;

         case DirectoryState::UNCACHED:
            assert(actual_main_l1d_cstate == CacheState::INVALID);
            assert(actual_pep_l1d_cstate == CacheState::INVALID);
            assert(actual_l2_cstate == CacheState::INVALID);
            break;

         default:
            assert(false);
            break;
      }
   }


   PrL1PrL1PrL2DramDirectoryMSI::MemoryManager* memory_manager = (PrL1PrL1PrL2DramDirectoryMSI::MemoryManager*) tile[0]->getMemoryManager();
   AddressHomeLookup* dram_dir_home_lookup = memory_manager->getDramDirectoryHomeLookup();
   tile_id_t home = dram_dir_home_lookup->getHome(address);

   PrL1PrL1PrL2DramDirectoryMSI::MemoryManager* home_memory_manager = (PrL1PrL1PrL2DramDirectoryMSI::MemoryManager*) tile[home]->getMemoryManager();
   PrL1PrL1PrL2DramDirectoryMSI::DramDirectoryCache* dram_dir_cache = home_memory_manager->getDramDirectoryCache();
   
   DirectoryEntryLimitedBroadcast* dir_entry = (DirectoryEntryLimitedBroadcast*) dram_dir_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* dir_block_info = dir_entry->getDirectoryBlockInfo();

   LOG_ASSERT_ERROR(dir_block_info->getDState() == dstate, "expected state(%u), actual state(%u)",
         dstate, dir_block_info->getDState());

   assert(dir_entry->getOwner() == owner_tile);

   LOG_ASSERT_ERROR(dir_entry->getNumSharers() == num_sharers, "dir_entry->numSharers(%u), num_sharers(%u)", dir_entry->getNumSharers(), num_sharers);
   for (tile_id_t i = 1; i <= 3; i++)
   {
      assert(dir_entry->hasSharer(i) == isSharer(i, tracked_sharers_list));
   }

   PrL1PrL1PrL2DramDirectoryMSI::DramCntlr* home_dram_cntlr = (PrL1PrL1PrL2DramDirectoryMSI::DramCntlr*) home_memory_manager->getDramCntlr();
   UInt32 cache_block_size = home_memory_manager->getCacheBlockSize();
   Byte data_buf[cache_block_size];
   home_dram_cntlr->getDataFromDram(address, Sim()->getTileManager()->getCurrentTile()->getId(), data_buf);
   int actual_memory_val = *((int*) &data_buf[address & (cache_block_size-1)]);

   assert(actual_memory_val == memory_val);
}

bool isSharer(tile_id_t i, int tracked_sharers_list)
{
   if (tracked_sharers_list & (1 << i))
      return true;
   else
      return false;
}
