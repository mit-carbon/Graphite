#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <boost/lexical_cast.hpp>
using namespace std;

#include "directory_cache.h"
#include "simulator.h"
#include "config.h"
#include "log.h"
#include "utils.h"

#define DETAILED_TRACKING_ENABLED    1

DirectoryCache::DirectoryCache(Tile* tile,
                               string directory_type_str,
                               UInt32 total_entries,
                               UInt32 associativity,
                               UInt32 cache_block_size,
                               UInt32 max_hw_sharers,
                               UInt32 max_num_sharers,
                               UInt32 num_directory_caches,
                               UInt64 dram_directory_cache_access_delay_in_clock_cycles,
                               ShmemPerfModel* shmem_perf_model):
   m_tile(tile),
   m_total_entries(total_entries),
   m_associativity(associativity),
   m_cache_block_size(cache_block_size),
   m_dram_directory_cache_access_delay_in_clock_cycles(dram_directory_cache_access_delay_in_clock_cycles),
   m_shmem_perf_model(shmem_perf_model),
   m_cache_power_model(NULL),
   m_cache_area_model(NULL),
   m_total_directory_cache_accesses(0),
   m_enabled(false)
{
   LOG_PRINT("Directory Cache ctor enter");
   m_num_sets = m_total_entries / m_associativity;
   
   // Instantiate the directory
   m_directory = new Directory(directory_type_str, total_entries, max_hw_sharers, max_num_sharers);

   initializeParameters(num_directory_caches);
  
   volatile float core_frequency = Config::getSingleton()->getCoreFrequency(m_tile->getMainCoreId());
   LOG_PRINT("Got Core Frequency");

   // Size of each directory entry (in bits)
   UInt32 directory_entry_size = m_directory->getDirectoryEntrySize();
   LOG_PRINT("Got Directory Entry Size");

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      m_cache_power_model = new CachePowerModel("directory", ceil(m_total_entries * directory_entry_size / 8),
            directory_entry_size, m_associativity, m_dram_directory_cache_access_delay_in_clock_cycles, core_frequency);
   }
   if (Config::getSingleton()->getEnableAreaModeling())
   {
      m_cache_area_model = new CacheAreaModel("directory", ceil(m_total_entries * directory_entry_size / 8),
            directory_entry_size, m_associativity, m_dram_directory_cache_access_delay_in_clock_cycles, core_frequency);
   }
   LOG_PRINT("Directory Cache ctor exit");
}

DirectoryCache::~DirectoryCache()
{
   delete m_directory;
}

void
DirectoryCache::initializeParameters(UInt32 num_directory_caches)
{
   LOG_PRINT("initializeParameters() enter");
   UInt32 num_tiles = Config::getSingleton()->getTotalTiles();

   m_log_num_sets = floorLog2(m_num_sets);
   m_log_cache_block_size = floorLog2(m_cache_block_size);

   m_log_num_tiles = floorLog2(num_tiles);
   m_log_num_directory_caches = ceilLog2(num_directory_caches); 

   IntPtr stack_size = boost::lexical_cast<IntPtr> (Sim()->getCfg()->get("stack/stack_size_per_core"));
   LOG_ASSERT_ERROR(isPower2(stack_size), "stack_size(%#llx) should be a power of 2", stack_size);
   m_log_stack_size = floorLog2(stack_size);
   
#ifdef DETAILED_TRACKING_ENABLED
   m_set_specific_address_map.resize(m_num_sets);
   m_set_replacement_histogram.resize(m_num_sets);
#endif
   LOG_PRINT("initializeParameters() exit");
}

DirectoryEntry*
DirectoryCache::getDirectoryEntry(IntPtr address)
{
   // Update Performance Model
   if (getShmemPerfModel())
      getShmemPerfModel()->incrCycleCount(m_dram_directory_cache_access_delay_in_clock_cycles);

   if (m_enabled)
   {
      // Update Event & Dynamic Energy Counters
      if (Config::getSingleton()->getEnablePowerModeling())
         m_cache_power_model->updateDynamicEnergy();
      m_total_directory_cache_accesses ++;
   }

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
         if (getShmemPerfModel())
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
DirectoryCache::getReplacementCandidates(IntPtr address, vector<DirectoryEntry*>& replacement_candidate_list)
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
DirectoryCache::replaceDirectoryEntry(IntPtr replaced_address, IntPtr address)
{
   // Update Performance Model
   if (getShmemPerfModel())
      getShmemPerfModel()->incrCycleCount(m_dram_directory_cache_access_delay_in_clock_cycles);

   if (m_enabled)
   {
      // Update Event & Dynamic Energy Counters
      if (Config::getSingleton()->getEnablePowerModeling())
         m_cache_power_model->updateDynamicEnergy();
      m_total_directory_cache_accesses ++;
   }

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

#ifdef DETAILED_TRACKING_ENABLED
         m_replaced_address_map[replaced_address] ++;
         m_set_replacement_histogram[set_index] ++;
#endif

         return directory_entry;
      }
   }

   // Should not reach here
   assert(false);
}

void
DirectoryCache::invalidateDirectoryEntry(IntPtr address)
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
DirectoryCache::splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index)
{
   IntPtr cache_block_address = address >> (m_log_cache_block_size + m_log_num_directory_caches);

   tag = cache_block_address;
   set_index = computeSetIndex(address);
  
#ifdef DETAILED_TRACKING_ENABLED 
   m_address_map[address] ++;
   m_set_specific_address_map[set_index][address] ++;
#endif
}

IntPtr
DirectoryCache::computeSetIndex(IntPtr address)
{
   UInt32 num_tile_id_bits = (m_log_num_tiles <= m_log_num_sets) ? m_log_num_tiles : m_log_num_sets;
   IntPtr tile_id_bits = (address >> m_log_stack_size) & ((1 << num_tile_id_bits) - 1);

   UInt32 log_num_sub_block_bits = m_log_num_sets - num_tile_id_bits;

   IntPtr sub_block_id = (address >> (m_log_cache_block_size + m_log_num_directory_caches)) \
                         & ((1 << log_num_sub_block_bits) - 1);

   IntPtr super_block_id = (address >> (m_log_cache_block_size + m_log_num_directory_caches + log_num_sub_block_bits)) \
                           & ((1 << num_tile_id_bits) - 1);

   return ((tile_id_bits ^ super_block_id) << log_num_sub_block_bits) + sub_block_id;
}

void
DirectoryCache::outputSummary(ostream& out)
{
   if (m_tile->getId() == 0)
   {
      SInt32 num_tiles = Config::getSingleton()->getTotalTiles();
      SInt32 num_directory_caches = 0;
      UInt64 l2_cache_size = 0;
      try
      {
         num_directory_caches = (UInt32) Sim()->getCfg()->getInt("perf_model/dram/num_controllers");
         l2_cache_size = (UInt64) Sim()->getCfg()->getInt("perf_model/l2_cache/" + \
               Config::getSingleton()->getL2CacheType(m_tile->getId()) + "/cache_size");
      }
      catch (...)
      {
         LOG_PRINT_ERROR("Could not read parameters from cfg file");
      }
      if (num_directory_caches == -1)
         num_directory_caches = num_tiles;

      UInt64 expected_entries_per_directory_cache = (num_tiles * l2_cache_size * 1024 / m_cache_block_size) / num_directory_caches;
      // Convert to a power of 2
      expected_entries_per_directory_cache = UInt64(1) << floorLog2(expected_entries_per_directory_cache);

      if (m_total_entries < (expected_entries_per_directory_cache / 2))
      {
         LOG_PRINT_WARNING("Dram Directory under-provisioned, use \"perf_model/dram_directory/total_entries\" = %u",
               (expected_entries_per_directory_cache / 2));
      }
      else if (m_total_entries > (expected_entries_per_directory_cache * 2))
      {
         LOG_PRINT_WARNING("Dram Directory over-provisioned, use \"perf_model/dram_directory/total_entries\" = %u",
               (expected_entries_per_directory_cache * 2));
      }
   }

   out << "    Total Directory Cache Accesses: " << m_total_directory_cache_accesses << endl;
   // The power and area model summary
   if (Config::getSingleton()->getEnablePowerModeling())
      m_cache_power_model->outputSummary(out);
   if (Config::getSingleton()->getEnableAreaModeling())
      m_cache_area_model->outputSummary(out);

#ifdef DETAILED_TRACKING_ENABLED

   UInt32 max_set_size = 0;
   UInt32 min_set_size = 100000000;
   SInt32 set_index_with_max_size = -1;
   SInt32 set_index_with_min_size = -1;

   UInt64 total_evictions = 0;
   UInt64 max_set_evictions = 0;
   SInt32 set_index_with_max_evictions = -1;

   UInt64 max_address_evictions = 0;
   IntPtr address_with_max_evictions = INVALID_ADDRESS;
   
   for (UInt32 i = 0; i < m_num_sets; i++)
   {
      // max, min, average set size, set evictions
      if (m_set_specific_address_map[i].size() > max_set_size)
      {
         max_set_size = m_set_specific_address_map[i].size();
         set_index_with_max_size = i;
      }
      if (m_set_specific_address_map[i].size() < min_set_size)
      {
         min_set_size = m_set_specific_address_map[i].size();
         set_index_with_min_size = i;
      }
      if (m_set_replacement_histogram[i] > max_set_evictions)
      {
         max_set_evictions = m_set_replacement_histogram[i];
         set_index_with_max_evictions = i;
      }
      total_evictions += m_set_replacement_histogram[i];
   }

   for (map<IntPtr,UInt64>::iterator it = m_replaced_address_map.begin(); \
         it != m_replaced_address_map.end(); it++)
   {
      if ((*it).second > max_address_evictions)
      {
         max_address_evictions = (*it).second;
         address_with_max_evictions = (*it).first;
      }
   }

   // Total Number of Addresses
   // Max Set Size, Average Set Size, Min Set Size
   // Evictions: Average per set, Max, Address with max evictions
   out << "    Detailed Counters: " << endl;
   out << "      Total Addresses: " << m_address_map.size() << endl;

   out << "      Average set size: " << float(m_address_map.size()) / m_num_sets << endl;
   out << "      Set index with max size: " << set_index_with_max_size << endl;
   out << "      Max set size: " << max_set_size << endl;
   out << "      Set index with min size: " << set_index_with_min_size << endl;
   out << "      Min set size: " << min_set_size << endl;

   out << "      Average evictions per set: " << float(total_evictions) / m_num_sets << endl;
   out << "      Set index with max evictions: " << set_index_with_max_evictions << endl;
   out << "      Max set evictions: " << max_set_evictions << endl;
   
   out << "      Address with max evictions: 0x" << hex << address_with_max_evictions << dec << endl;
   out << "      Max address evictions: " << max_address_evictions << endl;
#endif
}

void
DirectoryCache::dummyOutputSummary(ostream& out)
{
   out << "    Total Directory Cache Accesses: " << endl;
   // The power and area model summary
   if (Config::getSingleton()->getEnablePowerModeling())
      CachePowerModel::dummyOutputSummary(out);
   if (Config::getSingleton()->getEnableAreaModeling())
      CacheAreaModel::dummyOutputSummary(out);

#ifdef DETAILED_TRACKING_ENABLED
   out << "    Detailed Counters: " << endl;
   out << "      Total Addresses: " << endl;

   out << "      Average set size: " << endl;
   out << "      Set index with max size: " << endl;
   out << "      Max set size: " << endl;
   out << "      Set index with min size: " << endl;
   out << "      Min set size: " << endl;

   out << "      Average evictions per set: " << endl;
   out << "      Set index with max evictions: " << endl;
   out << "      Max set evictions: " << endl;
   
   out << "      Address with max evictions: " << endl;
   out << "      Max address evictions: " << endl;
#endif
}
