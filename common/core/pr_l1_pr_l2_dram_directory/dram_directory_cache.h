#ifndef __DRAM_DIRECTORY_CACHE_H__
#define __DRAM_DIRECTORY_CACHE_H__

#include <string>
#include <vector>

#include "directory.h"
#include "shmem_perf_model.h"

namespace PrL1PrL2DramDirectory
{
   class DramDirectoryCache
   {
      private:
         Directory* m_directory;
         std::vector<DirectoryEntry*> m_replaced_directory_entry_list;

         UInt32 m_total_entries;
         UInt32 m_associativity;

         UInt32 m_num_sets;
         UInt32 m_cache_block_size;
         UInt32 m_log_num_sets;
         UInt32 m_log_cache_block_size;

         UInt32 m_dram_directory_cache_access_time;
         ShmemPerfModel* m_shmem_perf_model;

         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

         void splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index); 
         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         UInt32 getLogCacheBlockSize() { return m_log_cache_block_size; }
         UInt32 getNumSets() { return m_num_sets; }
         UInt32 getLogNumSets() { return m_log_num_sets; }
      
      public:

         DramDirectoryCache(std::string directory_type_str,
               UInt32 total_entries,
               UInt32 associativity,
               UInt32 cache_block_size,
               UInt32 max_hw_sharers,
               UInt32 max_num_sharers,
               UInt32 dram_directory_cache_access_time,
               ShmemPerfModel* shmem_perf_model);
         ~DramDirectoryCache();

         DirectoryEntry* getDirectoryEntry(IntPtr address);
         DirectoryEntry* replaceDirectoryEntry(IntPtr replaced_address, IntPtr address);
         void invalidateDirectoryEntry(IntPtr address);
         void getReplacementCandidates(IntPtr address, std::vector<DirectoryEntry*>& replacement_candidate_list); 

   };
}

#endif /* __DRAM_DIRECTORY_CACHE_H__ */
