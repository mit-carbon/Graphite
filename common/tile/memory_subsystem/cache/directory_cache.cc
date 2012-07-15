#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <math.h>
#include <boost/lexical_cast.hpp>
using namespace std;

#include "directory_cache.h"
#include "memory_manager.h"
#include "simulator.h"
#include "config.h"
#include "log.h"
#include "utils.h"

#define DETAILED_TRACKING_ENABLED    1

DirectoryCache::DirectoryCache(Tile* tile,
                               CachingProtocolType caching_protocol_type,
                               string directory_type_str,
                               UInt32 total_entries,
                               UInt32 associativity,
                               UInt32 cache_line_size,
                               UInt32 max_hw_sharers,
                               UInt32 max_num_sharers,
                               UInt32 num_directories,
                               UInt64 directory_access_delay_in_clock_cycles)
   : _tile(tile)
   , _caching_protocol_type(caching_protocol_type)
   , _max_hw_sharers(max_hw_sharers)
   , _max_num_sharers(max_num_sharers)
   , _total_entries(total_entries)
   , _associativity(associativity)
   , _cache_line_size(cache_line_size)
   , _num_directories(num_directories)
   , _directory_access_delay_in_clock_cycles(directory_access_delay_in_clock_cycles)
   , _power_model(NULL)
   , _area_model(NULL)
   , _enabled(false)
{
   LOG_PRINT("Directory Cache ctor enter");
   _num_sets = _total_entries / _associativity;
 
   // Parse the directory type (full_map, limited_no_broadcast, limited_broadcast, ackwise, limitless) 
   _directory_type = DirectoryEntry::parseDirectoryType(directory_type_str);

   // Instantiate the directory
   _directory = new Directory(caching_protocol_type, _directory_type, total_entries, max_hw_sharers, max_num_sharers);

   initializeParameters();
  
   float core_frequency = Config::getSingleton()->getCoreFrequency(_tile->getMainCoreId());
   LOG_PRINT("Got Core Frequency");

   // Size of each directory entry (in bits)
   UInt32 directory_entry_size = ceil(1.0 * DirectoryEntry::getSize(_directory_type, max_hw_sharers, max_num_sharers)  / 8);
   LOG_PRINT("Got Directory Entry Size");

   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _power_model = new CachePowerModel("directory", _total_entries * directory_entry_size,
            directory_entry_size, _associativity, _directory_access_delay_in_clock_cycles, core_frequency);
   }
   if (Config::getSingleton()->getEnableAreaModeling())
   {
      _area_model = new CacheAreaModel("directory", _total_entries * directory_entry_size,
            directory_entry_size, _associativity, _directory_access_delay_in_clock_cycles, core_frequency);
   }

   initializeEventCounters();
   LOG_PRINT("Directory Cache ctor exit");
}

DirectoryCache::~DirectoryCache()
{
   delete _directory;
}

void
DirectoryCache::initializeParameters()
{
   LOG_PRINT("initializeParameters() enter");
   UInt32 num_application_tiles = Config::getSingleton()->getApplicationTiles();

   _log_num_sets = floorLog2(_num_sets);
   _log_cache_line_size = floorLog2(_cache_line_size);

   _log_num_application_tiles = floorLog2(num_application_tiles);
   _log_num_directories = ceilLog2(_num_directories); 

   IntPtr stack_size = boost::lexical_cast<IntPtr> (Sim()->getCfg()->get("stack/stack_size_per_core"));
   LOG_ASSERT_ERROR(isPower2(stack_size), "stack_size(%#lx) should be a power of 2", stack_size);
   _log_stack_size = floorLog2(stack_size);
   
#ifdef DETAILED_TRACKING_ENABLED
   _set_specific_address_map.resize(_num_sets);
   _set_replacement_histogram.resize(_num_sets);
#endif
   LOG_PRINT("initializeParameters() exit");
}

void
DirectoryCache::initializeEventCounters()
{
   _total_directory_accesses = 0;
   _tag_array_reads = 0;
   _tag_array_writes = 0;
   _data_array_reads = 0;
   _data_array_writes = 0;
}

void
DirectoryCache::updateCounters()
{
   _total_directory_accesses ++;
   
   // Update tag and data array reads/writes
   _tag_array_reads ++;
   _tag_array_writes ++;
   _data_array_reads ++;
   _data_array_writes ++;
   
   // Update dynamic energy counters
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->updateDynamicEnergy();
      
}

DirectoryEntry*
DirectoryCache::getDirectoryEntry(IntPtr address)
{
   if (_enabled)
   {
      // Update Performance Model
      getShmemPerfModel()->incrCycleCount(_directory_access_delay_in_clock_cycles);
      // Update event & dynamic energy counters
      updateCounters();
   }

   IntPtr tag;
   UInt32 set_index;
   
   // Assume that it always hit in the Dram Directory Cache for now
   splitAddress(address, tag, set_index);
   
   // Find the relevant directory entry
   for (UInt32 i = 0; i < _associativity; i++)
   {
      DirectoryEntry* directory_entry = _directory->getDirectoryEntry(set_index * _associativity + i);

      if (directory_entry->getAddress() == address)
      {
         if (getShmemPerfModel())
            getShmemPerfModel()->incrCycleCount(directory_entry->getLatency());
         // Simple check for now. Make sophisticated later
         return directory_entry;
      }
   }

   // Find a free directory entry if one does not currently exist
   for (UInt32 i = 0; i < _associativity; i++)
   {
      DirectoryEntry* directory_entry = _directory->getDirectoryEntry(set_index * _associativity + i);
      if (directory_entry->getAddress() == INVALID_ADDRESS)
      {
         // Simple check for now. Make sophisticated later
         directory_entry->setAddress(address);
         return directory_entry;
      }
   }

   // Check in the _replaced_directory_entry_list
   vector<DirectoryEntry*>::iterator it;
   for (it = _replaced_directory_entry_list.begin(); it != _replaced_directory_entry_list.end(); it++)
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

   for (UInt32 i = 0; i < _associativity; i++)
   {
      replacement_candidate_list.push_back(_directory->getDirectoryEntry(set_index * _associativity + i));
   }
}

DirectoryEntry*
DirectoryCache::replaceDirectoryEntry(IntPtr replaced_address, IntPtr address)
{
   if (_enabled)
   {
      // Update Performance Model
      getShmemPerfModel()->incrCycleCount(_directory_access_delay_in_clock_cycles);
      // Update event & dynamic energy counters
      updateCounters();
   }

   IntPtr tag;
   UInt32 set_index;
   splitAddress(replaced_address, tag, set_index);

   for (UInt32 i = 0; i < _associativity; i++)
   {
      DirectoryEntry* replaced_directory_entry = _directory->getDirectoryEntry(set_index * _associativity + i);
      if (replaced_directory_entry->getAddress() == replaced_address)
      {
         _replaced_directory_entry_list.push_back(replaced_directory_entry);

         DirectoryEntry* directory_entry = DirectoryEntry::create(_caching_protocol_type, _directory_type, _max_hw_sharers, _max_num_sharers);
         directory_entry->setAddress(address);
         _directory->setDirectoryEntry(set_index * _associativity + i, directory_entry);

#ifdef DETAILED_TRACKING_ENABLED
         _replaced_address_map[replaced_address] ++;
         _set_replacement_histogram[set_index] ++;
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
   for (it = _replaced_directory_entry_list.begin(); it != _replaced_directory_entry_list.end(); it++)
   {
      if ((*it)->getAddress() == address)
      {
         delete (*it);
         _replaced_directory_entry_list.erase(it);

         return;
      }
   }

   // Should not reach here
   assert(false);
}

void
DirectoryCache::splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index)
{
   IntPtr cache_line_address = address >> _log_cache_line_size;

   tag = cache_line_address;
   set_index = computeSetIndex(address);
  
#ifdef DETAILED_TRACKING_ENABLED 
   _address_map[address] ++;
   _set_specific_address_map[set_index][address] ++;
#endif
}

IntPtr
DirectoryCache::computeSetIndex(IntPtr address)
{
   LOG_PRINT("Computing Set for address(%#lx), _log_cache_line_size(%u), _log_num_sets(%u), _log_num_directories(%u)",
             address, _log_cache_line_size, _log_num_sets, _log_num_directories);

   IntPtr set = 0;
   for (UInt32 i = _log_cache_line_size + _log_num_directories; (i + _log_num_sets) <= (sizeof(IntPtr)*8); i += _log_num_sets)
   {
      IntPtr addr_bits = getBits<IntPtr>(address, i + _log_num_sets, i);
      set = set ^ addr_bits;
   }

   LOG_PRINT("Set(%#x)", (UInt32) set);

   return (UInt32) set;
}

void
DirectoryCache::checkDirectorySize(tile_id_t tile_id)
{
   if (tile_id != 0)
      return;

   UInt32 total_entries = 0;
   SInt32 num_application_tiles = Config::getSingleton()->getApplicationTiles();
   string num_directories_str;
   UInt64 l2_cache_size = 0;
   UInt32 cache_line_size = 0;
   try
   {
      total_entries = Sim()->getCfg()->getInt("dram_directory/total_entries");
      num_directories_str = Sim()->getCfg()->getString("dram/num_controllers");
      l2_cache_size = (UInt64) Sim()->getCfg()->getInt("l2_cache/" +
                      Config::getSingleton()->getL2CacheType(tile_id) + "/cache_size");
      cache_line_size = (UInt64) Sim()->getCfg()->getInt("l2_cache/" +
                        Config::getSingleton()->getL2CacheType(tile_id) + "/cache_line_size");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read parameters from cfg file");
   }
   
   SInt32 num_directories = (num_directories_str == "ALL") ? num_application_tiles : convertFromString<UInt32>(num_directories_str);

   UInt64 expected_entries_per_directory = (num_application_tiles * l2_cache_size * 1024 / cache_line_size) / num_directories;
   // Convert to a power of 2
   expected_entries_per_directory = UInt64(1) << floorLog2(expected_entries_per_directory);

   if (total_entries < expected_entries_per_directory)
   {
      LOG_PRINT_WARNING("Dram Directory under-provisioned, use \"dram_directory/total_entries\" = %u",
            expected_entries_per_directory);
   }
   else if (total_entries > (expected_entries_per_directory * 2))
   {
      LOG_PRINT_WARNING("Dram Directory over-provisioned, use \"dram_directory/total_entries\" = %u",
            (expected_entries_per_directory * 2));
   }
}

void
DirectoryCache::outputSummary(ostream& out)
{
   checkDirectorySize(_tile->getId());

   out << "    Total Directory Accesses: " << _total_directory_accesses << endl;
   // The power and area model summary
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->outputSummary(out);
   if (Config::getSingleton()->getEnableAreaModeling())
      _area_model->outputSummary(out);
   out << "    Event Counters: " << endl;
   out << "      Tag Array Reads: " << _tag_array_reads << endl;
   out << "      Tag Array Writes: " << _tag_array_writes << endl;
   out << "      Data Array Reads: " << _data_array_reads << endl;
   out << "      Data Array Writes: " << _data_array_writes << endl;

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
   
   for (UInt32 i = 0; i < _num_sets; i++)
   {
      // max, min, average set size, set evictions
      if (_set_specific_address_map[i].size() > max_set_size)
      {
         max_set_size = _set_specific_address_map[i].size();
         set_index_with_max_size = i;
      }
      if (_set_specific_address_map[i].size() < min_set_size)
      {
         min_set_size = _set_specific_address_map[i].size();
         set_index_with_min_size = i;
      }
      if (_set_replacement_histogram[i] > max_set_evictions)
      {
         max_set_evictions = _set_replacement_histogram[i];
         set_index_with_max_evictions = i;
      }
      total_evictions += _set_replacement_histogram[i];
   }

   for (map<IntPtr,UInt64>::iterator it = _replaced_address_map.begin();
         it != _replaced_address_map.end(); it++)
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
   out << "      Total Addresses: " << _address_map.size() << endl;

   out << "      Average set size: " << float(_address_map.size()) / _num_sets << endl;
   out << "      Set index with max size: " << set_index_with_max_size << endl;
   out << "      Max set size: " << max_set_size << endl;
   out << "      Set index with min size: " << set_index_with_min_size << endl;
   out << "      Min set size: " << min_set_size << endl;

   out << "      Average evictions per set: " << float(total_evictions) / _num_sets << endl;
   out << "      Set index with max evictions: " << set_index_with_max_evictions << endl;
   out << "      Max set evictions: " << max_set_evictions << endl;
   
   out << "      Address with max evictions: 0x" << hex << address_with_max_evictions << dec << endl;
   out << "      Max address evictions: " << max_address_evictions << endl;
#endif
}

void
DirectoryCache::dummyOutputSummary(ostream& out, tile_id_t tile_id)
{
   checkDirectorySize(tile_id);

   out << "    Total Directory Cache Accesses: " << endl;
   // The power and area model summary
   if (Config::getSingleton()->getEnablePowerModeling())
      CachePowerModel::dummyOutputSummary(out);
   if (Config::getSingleton()->getEnableAreaModeling())
      CacheAreaModel::dummyOutputSummary(out);
   out << "    Event Counters: " << endl;
   out << "      Tag Array Reads: " << endl;
   out << "      Tag Array Writes: " << endl;
   out << "      Data Array Reads: " << endl;
   out << "      Data Array Writes: " << endl;

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

ShmemPerfModel*
DirectoryCache::getShmemPerfModel()
{
   return _tile->getMemoryManager()->getShmemPerfModel();
}
