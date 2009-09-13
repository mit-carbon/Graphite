#include "dram_directory_cache.h"
#include "log.h"
#include "utils.h"

namespace PrL1PrL2DramDirectory
{

DramDirectoryCache::DramDirectoryCache(
      std::string directory_type_str,
      UInt32 total_entries,
      UInt32 associativity,
      UInt32 cache_block_size,
      UInt32 max_hw_sharers,
      UInt32 max_num_sharers):
   m_total_entries(total_entries),
   m_associativity(associativity),
   m_cache_block_size(cache_block_size)
{
   m_num_sets = m_total_entries / m_associativity;
   m_sets = new IntPtr*[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      m_sets[i] = new IntPtr[m_associativity];
      for (UInt32 j = 0; j < m_associativity; j++)
         m_sets[i][j] = INVALID_ADDRESS;
   }
   // Instantiate the directory
   m_directory = new Directory(directory_type_str, total_entries, max_hw_sharers, max_num_sharers);

   // Logs
   m_log_num_sets = floorLog2(m_num_sets);
   m_log_cache_block_size = floorLog2(m_cache_block_size);
}

DramDirectoryCache::~DramDirectoryCache()
{
   for (UInt32 i = 0; i < m_num_sets; i++)
      delete [] m_sets[i];
   delete [] m_sets;

   delete m_directory;
}

DirectoryEntry*
DramDirectoryCache::getDirectoryEntry(IntPtr address)
{
   IntPtr tag;
   UInt32 set_index;
   
   // Assume that it always hit in the Dram Directory Cache for now
   splitAddress(address, tag, set_index);
   
   // Find the relevant directory entry
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_sets[set_index][i] == address)
      {
         // Simple check for now. Make sophisticated later
         return m_directory->getDirectoryEntry(set_index * m_associativity + i);
      }
   }

   // Find a free directory entry if one does not currently exist
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      if (m_sets[set_index][i] == INVALID_ADDRESS)
      {
         m_sets[set_index][i] = address;
         // Simple check for now. Make sophisticated later
         return m_directory->getDirectoryEntry(set_index * m_associativity + i);
      }
   }

   LOG_PRINT_ERROR("Assume that dram_directory_cache always hits. Fix later");
   return (DirectoryEntry*) NULL;
}

void
DramDirectoryCache::splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index)
{
   IntPtr cache_block_address = address >> getLogCacheBlockSize();
   tag = cache_block_address >> getLogNumSets();
   set_index = ((UInt32) cache_block_address) & (getNumSets() - 1);

}

}
