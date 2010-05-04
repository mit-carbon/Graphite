#include <sstream>
#include <fstream>
#include <iostream>
#include <iomanip>
using namespace std;

#include <boost/lexical_cast.hpp>
#include "memory_manager.h"
#include "dram_directory_cache.h"
#include "simulator.h"
#include "config.h"
#include "log.h"
#include "utils.h"

// #define TRACK_ADDRESSES    1

namespace PrL1PrL2DramDirectoryMSI
{

DramDirectoryCache::DramDirectoryCache(
      MemoryManager* memory_manager,
      string directory_type_str,
      UInt32 total_entries,
      UInt32 associativity,
      UInt32 cache_block_size,
      UInt32 max_hw_sharers,
      UInt32 max_num_sharers,
      UInt32 dram_directory_cache_access_time,
      UInt32 num_dram_cntlrs,
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

   initializeParameters(num_dram_cntlrs);
}

DramDirectoryCache::~DramDirectoryCache()
{
   delete m_directory;
}

void
DramDirectoryCache::initializeParameters(UInt32 num_dram_cntlrs)
{
   UInt32 num_cores = Config::getSingleton()->getTotalCores();

   m_log_num_sets = floorLog2(m_num_sets);
   m_log_cache_block_size = floorLog2(m_cache_block_size);
   m_log_num_cores = floorLog2(num_cores);
   
   if (isPower2(num_dram_cntlrs))
      m_log_num_dram_cntlrs = floorLog2(num_dram_cntlrs);
   else
      m_log_num_dram_cntlrs = 0;

   IntPtr stack_size = boost::lexical_cast<IntPtr> (Sim()->getCfg()->get("stack/stack_size_per_core"));
   LOG_ASSERT_ERROR(isPower2(stack_size), "stack_size(%#llx) should be a power of 2", stack_size);
   m_log_stack_size = floorLog2(stack_size);
   
#ifdef TRACK_ADDRESSES
   m_set_specific_address_set.resize(m_num_sets);
#endif
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
   vector<DirectoryEntry*>::iterator it;
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
DramDirectoryCache::getReplacementCandidates(IntPtr address, vector<DirectoryEntry*>& replacement_candidate_list)
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

         return directory_entry;
      }
   }

   // Should not reach here
   assert(false);
}

void
DramDirectoryCache::invalidateDirectoryEntry(IntPtr address)
{
   vector<DirectoryEntry*>::iterator it;
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
   IntPtr cache_block_address = address >> (m_log_cache_block_size + m_log_num_dram_cntlrs);

   tag = cache_block_address;
   set_index = computeSetIndex(address);
  
#ifdef TRACK_ADDRESSES 
   m_address_map[address] ++;
   m_set_specific_address_set[set_index].insert(address);
#endif
}

IntPtr
DramDirectoryCache::computeSetIndex(IntPtr address)
{
   IntPtr core_id = (address >> m_log_stack_size) & ((1 << m_log_num_cores) - 1);

   UInt32 log_num_sub_block_bits = m_log_num_sets - m_log_num_cores;
   IntPtr sub_block_id = (address >> (m_log_cache_block_size + m_log_num_dram_cntlrs)) \
                         & ((1 << log_num_sub_block_bits) - 1);

   IntPtr super_block_id = (address >> (m_log_cache_block_size + m_log_num_dram_cntlrs + log_num_sub_block_bits)) \
                           & ((1 << m_log_num_cores) - 1);

   return ((core_id ^ super_block_id) << log_num_sub_block_bits) + sub_block_id;
}

void
DramDirectoryCache::outputSummary(ostream& out)
{
   if (m_memory_manager->getCore()->getId() == 0)
   {
      SInt32 num_cores = Config::getSingleton()->getTotalCores();
      SInt32 num_dram_cntlrs = 0;
      UInt64 l2_cache_size = 0;
      try
      {
         num_dram_cntlrs = (UInt32) Sim()->getCfg()->getInt("perf_model/dram/num_controllers");
         l2_cache_size = (UInt64) Sim()->getCfg()->getInt("perf_model/l2_cache/cache_size");
      }
      catch (...)
      {
         LOG_PRINT_ERROR("Could not read parameters from cfg file");
      }
      if (num_dram_cntlrs == -1)
         num_dram_cntlrs = num_cores;

      UInt64 expected_entries_per_dram_cntlr = (num_cores * l2_cache_size * 1024 / m_cache_block_size) / num_dram_cntlrs;
      // Convert to a power of 2
      expected_entries_per_dram_cntlr = UInt64(1) << floorLog2(expected_entries_per_dram_cntlr);

      if (m_total_entries < (expected_entries_per_dram_cntlr / 2))
      {
         LOG_PRINT_WARNING("Dram Directory under-provisioned, use \"perf_model/dram_directory/total_entries\" = %u",
               (expected_entries_per_dram_cntlr / 2));
      }
      else if (m_total_entries > (expected_entries_per_dram_cntlr * 2))
      {
         LOG_PRINT_WARNING("Dram Directory over-provisioned, use \"perf_model/dram_directory/total_entries\" = %u",
               (expected_entries_per_dram_cntlr * 2));
      }
   }

#ifdef TRACK_ADDRESSES
   core_id_t core_id = m_memory_manager->getCore()->getId();
   string output_dir = Sim()->getCfg()->getString("general/output_dir", "");

   ostringstream filename;
   filename << output_dir << "/address_set_" << core_id;
   ofstream address_set_file(filename.str().c_str());

   map<IntPtr,UInt64>::iterator it;
   for (it = m_address_map.begin(); it != m_address_map.end(); it++)
      address_set_file << setfill(' ') << setw(20) << left << hex << (*it).first << dec
         << setw(10) << left << (*it).second << "\n";
   address_set_file.close();

   UInt32 max_set_size = 0;
   UInt32 min_set_size = 1000000;
   UInt32 max_set_index = 1000000;
   UInt32 min_set_index = 1000000;

   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      // max, min, average
      if (m_set_specific_address_set[i].size() >= max_set_size)
      {
         max_set_size = m_set_specific_address_set[i].size();
         max_set_index = i;
      }
      if (m_set_specific_address_set[i].size() <= min_set_size)
      {
         min_set_size = m_set_specific_address_set[i].size();
         min_set_index = i;
      }
   }

   LOG_PRINT_WARNING("max(%u), max_index(%u), min(%u), min_index(%u), average(%f)",
         max_set_size, max_set_index, min_set_size, min_set_index, float(m_address_map.size()) / m_num_sets);
   set<IntPtr>::iterator set_it = m_set_specific_address_set[max_set_index].begin();
   for (; set_it != m_set_specific_address_set[max_set_index].end(); set_it++)
   {
      LOG_PRINT_WARNING("Address (%#llx)", *set_it);
   }
#endif
}

}
