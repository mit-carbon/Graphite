#pragma once

#include <string>
#include <map>
#include <vector>
#include <set>
#include "tile.h"
#include "directory.h"
#include "shmem_perf_model.h"
#include "cache_power_model.h"
#include "cache_area_model.h"

class DirectoryCache
{
public:
   DirectoryCache(Tile* tile,
                  std::string directory_type_str,
                  UInt32 total_entries,
                  UInt32 associativity,
                  UInt32 cache_block_size,
                  UInt32 max_hw_sharers,
                  UInt32 max_num_sharers,
                  UInt32 num_dram_cntlrs,
                  UInt64 dram_directory_cache_access_delay_in_clock_cycles,
                  ShmemPerfModel* shmem_perf_model);
   ~DirectoryCache();

   DirectoryEntry* getDirectoryEntry(IntPtr address);
   DirectoryEntry* replaceDirectoryEntry(IntPtr replaced_address, IntPtr address);
   void invalidateDirectoryEntry(IntPtr address);
   void getReplacementCandidates(IntPtr address, std::vector<DirectoryEntry*>& replacement_candidate_list);

   void outputSummary(std::ostream& os);
   static void dummyOutputSummary(std::ostream& os);

   void enable() { m_enabled = true; }
   void disable() { m_enabled = false; }

private:
   Tile* m_tile;
   Directory* m_directory;
   std::vector<DirectoryEntry*> m_replaced_directory_entry_list;
   
   std::map<IntPtr,UInt64> m_address_map;
   std::vector<std::map<IntPtr,UInt64> > m_set_specific_address_map;
   std::map<IntPtr,UInt64> m_replaced_address_map;
   std::vector<UInt64> m_set_replacement_histogram;

   UInt32 m_total_entries;
   UInt32 m_associativity;

   UInt32 m_num_sets;
   UInt32 m_cache_block_size;

   UInt32 m_log_num_sets;
   UInt32 m_log_cache_block_size;
   UInt32 m_log_num_tiles;
   UInt32 m_log_num_directory_caches;

   UInt32 m_log_stack_size;

   UInt64 m_dram_directory_cache_access_delay_in_clock_cycles;

   ShmemPerfModel* m_shmem_perf_model;

   // Dram Directory Cache Power and Area Models
   CachePowerModel* m_cache_power_model;
   CacheAreaModel* m_cache_area_model;

   // Counters
   UInt64 m_total_directory_cache_accesses;

   bool m_enabled;

   ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

   void initializeParameters(UInt32 num_dram_cntlrs);
   void splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index);
   
   IntPtr computeSetIndex(IntPtr address);
};
