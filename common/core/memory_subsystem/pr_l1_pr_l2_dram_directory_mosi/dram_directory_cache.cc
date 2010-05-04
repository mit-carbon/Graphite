#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;

#include "simulator.h"
#include "config.h"
#include "dram_directory_cache.h"
#include "config.h"
#include "log.h"
#include "utils.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectoryMOSI
{

DramDirectoryCache::DramDirectoryCache(
      MemoryManager* memory_manager,
      std::string directory_type_str,
      UInt32 total_entries,
      UInt32 associativity,
      UInt32 cache_block_size,
      UInt32 max_hw_sharers,
      UInt32 max_num_sharers,
      UInt32 num_dram_cntlrs,
      UInt32 dram_directory_cache_access_time,
      ShmemPerfModel* shmem_perf_model):
   m_memory_manager(memory_manager),
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

   // Get the number of cores
   m_log_num_cores = (UInt32) floorLog2(Config::getSingleton()->getTotalCores());

   // Get the number of Dram Cntlrs
   if (isPower2(num_dram_cntlrs))
      m_log_num_dram_cntlrs = (UInt32) floorLog2(num_dram_cntlrs);
   else
      m_log_num_dram_cntlrs = 0;

   histogram = new UInt64[m_num_sets];
   for (UInt32 i = 0; i < m_num_sets; i++)
      histogram[i] = 0;

   initializeRandomBitsTracker();
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

      if (directory_entry->getAddress() == address)
      {
         if (m_shmem_perf_model)
            getShmemPerfModel()->incrCycleCount(directory_entry->getLatency());
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
   if (m_shmem_perf_model)
      getShmemPerfModel()->incrCycleCount(m_dram_directory_cache_access_time);

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
   // Lets have some hard-coded values here
   // Take them out later
   IntPtr cache_block_address = address >> getLogCacheBlockSize();
   tag = address;
 
   /* 
   {
      // Intelligent way of hashing
      // getLogNumCores, getLogNumDramCntlrs
      cache_block_address = cache_block_address >> getLogNumDramCntlrs();

      set_index = 0;
      for (UInt32 i = ADDRESS_THRESHOLD; i <= (8*sizeof(IntPtr) - getLogNumCores()); i = i + getLogNumCores())
      {
         set_index = set_index ^ selectBits(address, i, i + getLogNumCores());
      }
      set_index = (set_index << (getLogNumSets() - getLogNumCores())) + selectBits(cache_block_address, 0, getLogNumSets() - getLogNumCores());

      // LOG_PRINT_ERROR("Address(%#llx), set_index(%#x), ADDRESS_THRESHOLD(%u), logNumDramCntlrs(%u), logNumCores(%u), logNumSets(%u)",
      //       address, set_index, ADDRESS_THRESHOLD, getLogNumDramCntlrs(), getLogNumCores(), getLogNumSets());
   }
   */

   {
      // Stupid way of hashing
      processRandomBits(address);

      cache_block_address = cache_block_address >> getLogNumDramCntlrs();
      set_index = ((UInt32) cache_block_address) & (getNumSets() - 1);
   }
}

void
DramDirectoryCache::initializeRandomBitsTracker()
{
   m_address_histogram_one.resize(sizeof(IntPtr) * 8);
   m_address_histogram.resize(sizeof(IntPtr) * 8);
}

void
DramDirectoryCache::processRandomBits(IntPtr address)
{
   if (m_global_address_set.find(address) != m_global_address_set.end())
      return;

   m_global_address_set.insert(address);

   for (UInt32 i = 0; i < (8 * sizeof(IntPtr)); i++)
   {
      // Get the ith bit from address
      UInt64 bit = (address >> i) & 0x1;
      assert((bit == 0) || (bit == 1));

      m_address_histogram_one[i] += bit;
      m_address_histogram[i] += 1;
   } 
}

void
DramDirectoryCache::printRandomBitsHistogram()
{
   core_id_t core_id = getMemoryManager()->getCore()->getId();
   
   ostringstream file_name;
   file_name << Sim()->getCfg()->getString("general/output_dir") << "address_histogram_" << core_id;

   ofstream output_file((file_name.str()).c_str());
   for (UInt32 i = 0; i < (8 * sizeof(IntPtr)); i++)
   {
      // LOG_PRINT_WARNING("printRandomBitsHistogram() (%u) - (%llu, %llu)", i, m_address_histogram_one[i], m_address_histogram[i]);
      if (m_address_histogram[i] > 0)
         output_file << i << "\t" << ((float) m_address_histogram_one[i]) / m_address_histogram[i] << endl;
      else
         output_file << i << "\t" << "NA" << endl;
   }
   output_file.close();
}

UInt32
DramDirectoryCache::selectBits(IntPtr address, UInt32 low, UInt32 high)
{
   // Includes low but does not include high - (high, low]
   LOG_ASSERT_ERROR((low < high) && ((high-low) <= (sizeof(UInt32)*8)),
         "low(%u), high(%u), address(%#llx)", low, high, address);
   return (address >> low) & ((2 << (high-low)) - 1); 
}

void
DramDirectoryCache::outputSummary(std::ostream& out)
{
   UInt64 mean, total, max;
   SInt32 max_index;
   IntPtr max_replaced_address;
   SInt32 max_replaced_times;

   // printRandomBitsHistogram();

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
