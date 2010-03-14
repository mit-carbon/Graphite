#include "dram_directory_cache.h"
#include "log.h"
#include "utils.h"

namespace PrL1PrL2DramDirectoryMOSI
{

DramDirectoryCache::DramDirectoryCache(
      std::string directory_type_str,
      UInt32 total_entries,
      UInt32 associativity,
      UInt32 cache_block_size,
      UInt32 max_hw_sharers,
      UInt32 max_num_sharers,
      UInt32 num_dram_cntlrs,
      UInt32 dram_directory_cache_access_time,
      ShmemPerfModel* shmem_perf_model):
   m_total_entries(total_entries),
   m_associativity(associativity),
   m_cache_block_size(cache_block_size),
   m_dram_directory_cache_access_time(dram_directory_cache_access_time),
   m_shmem_perf_model(shmem_perf_model)
{
   m_num_sets = m_total_entries / m_associativity;
   
   // Instantiate the directory
   m_directory = new Directory(directory_type_str, total_entries, max_hw_sharers, max_num_sharers);

   // Logs
   m_log_num_sets = floorLog2(m_num_sets);
   m_log_cache_block_size = floorLog2(m_cache_block_size);

   // Get the number of Dram Cntlrs
   if (isPower2(num_dram_cntlrs))
      m_log_num_dram_cntlrs = (UInt32) floorLog2(num_dram_cntlrs);
   else
      m_log_num_dram_cntlrs = 0;

   histogram = new UInt64[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
      histogram[i] = 0;
}

DramDirectoryCache::~DramDirectoryCache()
{
   delete m_directory;
}

DirectoryEntry*
DramDirectoryCache::getDirectoryEntry(IntPtr address)
{
   if (m_shmem_perf_model)
      getShmemPerfModel()->incrCycleCount(m_dram_directory_cache_access_time);

   IntPtr tag;
   UInt32 set_index;
   
   // Assume that it always hit in the Dram Directory Cache for now
   splitAddress(address, tag, set_index);
   
   // Find the relevant directory entry
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      DirectoryEntry* directory_entry = m_directory->getDirectoryEntry(set_index * m_associativity + i);
      if (m_shmem_perf_model)
         getShmemPerfModel()->incrCycleCount(directory_entry->getLatency());

      if (directory_entry->getAddress() == address)
      {
         // Simple check for now. Make sophisticated later
         return directory_entry;
      }
   }

   // Find a free directory entry if one does not currently exist
   for (UInt32 i = 0; i < m_associativity; i++)
   {
      DirectoryEntry* directory_entry = m_directory->getDirectoryEntry(set_index * m_associativity + i);
      if (directory_entry->getAddress() == INVALID_ADDRESS)
      {
         // Simple check for now. Make sophisticated later
         directory_entry->setAddress(address);
         return directory_entry;
      }
   }

   // Check in the m_replaced_directory_entry_list
   std::vector<DirectoryEntry*>::iterator it;
   for (it = m_replaced_directory_entry_list.begin(); it != m_replaced_directory_entry_list.end(); it++)
   {
      if ((*it)->getAddress() == address)
      {
         return (*it);
      } 
   }

   return (DirectoryEntry*) NULL;
}

void
DramDirectoryCache::getReplacementCandidates(IntPtr address, std::vector<DirectoryEntry*>& replacement_candidate_list)
{
   assert(getDirectoryEntry(address) == NULL);
   
   IntPtr tag;
   UInt32 set_index;
   splitAddress(address, tag, set_index);

   for (UInt32 i = 0; i < m_associativity; i++)
   {
      replacement_candidate_list.push_back(m_directory->getDirectoryEntry(set_index * m_associativity + i));
   }
}

DirectoryEntry*
DramDirectoryCache::replaceDirectoryEntry(IntPtr replaced_address, IntPtr address)
{
   IntPtr tag;
   UInt32 set_index;
   splitAddress(replaced_address, tag, set_index);

   for (UInt32 i = 0; i < m_associativity; i++)
   {
      DirectoryEntry* replaced_directory_entry = m_directory->getDirectoryEntry(set_index * m_associativity + i);
      if (replaced_directory_entry->getAddress() == replaced_address)
      {
         m_replaced_directory_entry_list.push_back(replaced_directory_entry);

         DirectoryEntry* directory_entry = m_directory->createDirectoryEntry();
         directory_entry->setAddress(address);
         m_directory->setDirectoryEntry(set_index * m_associativity + i, directory_entry);

         m_replaced_address_list[replaced_address] ++;
         histogram[set_index]++;

         return directory_entry;
      }
   }

   // Should not reach here
   assert(false);
}

void
DramDirectoryCache::invalidateDirectoryEntry(IntPtr address)
{
   std::vector<DirectoryEntry*>::iterator it;
   for (it = m_replaced_directory_entry_list.begin(); it != m_replaced_directory_entry_list.end(); it++)
   {
      if ((*it)->getAddress() == address)
      {
         delete (*it);
         m_replaced_directory_entry_list.erase(it);

         return;
      } 
   }

   // Should not reach here
   assert(false);
}

void
DramDirectoryCache::splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index)
{
   IntPtr cache_block_address = address >> getLogCacheBlockSize();
   tag = cache_block_address >> getLogNumSets();
   set_index = ((UInt32) cache_block_address) & (getNumSets() - 1);
}

void
DramDirectoryCache::outputSummary(std::ostream& out)
{
   UInt64 mean, total, max;
   SInt32 max_index;
   IntPtr max_replaced_address;
   SInt32 max_replaced_times;

   aggregateStatistics(histogram, mean, total, max, max_index, max_replaced_address, max_replaced_times);

   out << "Dram Directory Cache: " << endl;
   out << "    mean evictions: " << mean << endl;
   out << "    total evictions: " << total << endl;
   out << "    max evictions: " << max << endl;
   out << "    max replaced index: " << max_index << endl;
   out << "    max replaced address: 0x" << hex << max_replaced_address << dec << endl;
   out << "    max replaced times: " << max_replaced_times << endl;
}

void
DramDirectoryCache::dummyOutputSummary(std::ostream& out)
{
   out << "Dram Directory Cache: " << endl;
   out << "    mean evictions: NA" << endl;
   out << "    total evictions: NA" << endl;
   out << "    max evictions: NA" << endl;
   out << "    max replaced index: NA" << endl;
   out << "    max replaced address: NA" << endl;
   out << "    max replaced times: NA" << endl;
}

void
DramDirectoryCache::aggregateStatistics(UInt64* histogram, UInt64& mean, UInt64& total, UInt64& max, SInt32& max_index, IntPtr& max_replaced_address, SInt32& max_replaced_times)
{
   mean = total = max = 0;
   max_index = -1;
   max_replaced_address = INVALID_ADDRESS;
   max_replaced_times = 0;

   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      if (histogram[i] > max)
      {
         max = histogram[i];
         max_index = i;
      }
      total += histogram[i];
   }

   map<IntPtr, SInt32>::iterator it;
   for (it = m_replaced_address_list.begin(); it != m_replaced_address_list.end(); it++)
   {
      if ((*it).second > max_replaced_times)
      {
         max_replaced_address = (*it).first;
         max_replaced_times = (*it).second;
      } 
   }
   mean = total / m_num_sets;
}

}
