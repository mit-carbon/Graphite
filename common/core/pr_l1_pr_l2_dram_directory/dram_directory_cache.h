#ifndef __DRAM_DIRECTORY_CACHE_H__
#define __DRAM_DIRECTORY_CACHE_H__

#include <string>

#include "directory.h"

namespace PrL1PrL2DramDirectory
{
   class DramDirectoryCache
   {
      private:
         Directory* m_directory;

         UInt32 m_total_entries;
         UInt32 m_associativity;

         UInt32 m_num_sets;
         UInt32 m_cache_block_size;
         UInt32 m_log_num_sets;
         UInt32 m_log_cache_block_size;

         IntPtr** m_sets;

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
               UInt32 max_num_sharers);
         ~DramDirectoryCache();

         DirectoryEntry* getDirectoryEntry(IntPtr address);

   };
}

#endif /* __DRAM_DIRECTORY_CACHE_H__ */
