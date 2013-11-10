#include <cmath>
#include "directory_cache.h"
#include "memory_manager.h"
#include "simulator.h"
#include "config.h"
#include "log.h"
#include "mcpat_cache_interface.h"
#include "utils.h"

DirectoryCache::DirectoryCache(Tile* tile,
                               CachingProtocolType caching_protocol_type,
                               string directory_type_str,
                               string total_entries_str,
                               UInt32 associativity,
                               UInt32 cache_line_size,
                               UInt32 max_hw_sharers,
                               UInt32 max_num_sharers,
                               UInt32 num_directory_slices,
                               string directory_access_cycles_str,
                               ShmemPerfModel* shmem_perf_model)
   : _tile(tile)
   , _caching_protocol_type(caching_protocol_type)
   , _max_hw_sharers(max_hw_sharers)
   , _max_num_sharers(max_num_sharers)
   , _total_entries_str(total_entries_str)
   , _associativity(associativity)
   , _cache_line_size(cache_line_size)
   , _num_directory_slices(num_directory_slices)
   , _directory_access_cycles_str(directory_access_cycles_str)
   , _mcpat_cache_interface(NULL)
   , _enabled(false)
   , _module(DIRECTORY)
   , _shmem_perf_model(shmem_perf_model)
{
   LOG_PRINT("Directory Cache ctor enter");
 
   // Parse the directory type (full_map, limited_no_broadcast, limited_broadcast, ackwise, limitless) 
   _directory_type = DirectoryEntry::parseDirectoryType(directory_type_str);

   // Determine total number of directory entries (either automatically or user specified)
   _total_entries = computeDirectoryTotalEntries();
   _num_sets = _total_entries / _associativity;

   // Instantiate the directory
   _directory = new Directory(caching_protocol_type, _directory_type, _total_entries, max_hw_sharers, max_num_sharers);

   // Size of each directory entry (in bytes)
   UInt32 max_application_sharers = Config::getSingleton()->getApplicationTiles();
   UInt32 directory_entry_size = ceil(1.0 * DirectoryEntry::getSize(_directory_type, max_hw_sharers, max_application_sharers)  / 8);
   _directory_size = _total_entries * directory_entry_size;

   //initialize frequency and voltage
   int rc = DVFSManager::getInitialFrequencyAndVoltage(DIRECTORY, _frequency, _voltage);
   LOG_ASSERT_ERROR(rc == 0, "Error setting initial voltage for frequency(%g)", _frequency);


   // Calculate access time based on size of directory entry and total number of entries (or) user specified
   _directory_access_cycles = computeDirectoryAccessCycles();
   _directory_access_latency = Time(Latency(_directory_access_cycles, _frequency));

   // asynchronous communication
   _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), _frequency));
   _asynchronous_map[L2_CACHE] = Time(0);
   _asynchronous_map[NETWORK_MEMORY] = Time(0);
  
   LOG_PRINT("Total Entries(%u), Entry Size(%u), Access Time(%llu)",
         _total_entries, directory_entry_size, _directory_access_latency.toNanosec());

   if (Config::getSingleton()->getEnablePowerModeling() || Config::getSingleton()->getEnableAreaModeling())
   {
      // _mcpat_cache_interface = new McPATCacheInterface(this, Sim()->getCfg()->getInt("general/technology_node"));
   }

   _log_num_sets = floorLog2(_num_sets);
   _log_cache_line_size = floorLog2(_cache_line_size);
   _log_num_directory_slices = ceilLog2(_num_directory_slices); 
   
   initializeEventCounters();

   LOG_PRINT("Directory Cache ctor exit");
}

DirectoryCache::~DirectoryCache()
{
   delete _directory;
}

void
DirectoryCache::initializeEventCounters()
{
   _total_directory_accesses = 0;
   _total_evictions = 0;
   _total_back_invalidations = 0;
}

void
DirectoryCache::updateCounters()
{
   _total_directory_accesses ++;
}

DirectoryEntry*
DirectoryCache::getDirectoryEntry(IntPtr address)
{
   if (_enabled)
   {
      // Update Performance Model
      getShmemPerfModel()->incrCurrTime(_directory_access_latency);
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
            getShmemPerfModel()->incrCurrTime(Latency(directory_entry->getLatency(),_frequency));
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
   IntPtr tag;
   UInt32 set_index;
   splitAddress(replaced_address, tag, set_index);

   DirectoryEntry* replaced_directory_entry = NULL;
   DirectoryEntry* new_directory_entry = DirectoryEntry::create(_caching_protocol_type, _directory_type, _max_hw_sharers, _max_num_sharers);
   new_directory_entry->setAddress(address);

   for (UInt32 i = 0; i < _associativity; i++)
   {
      DirectoryEntry* directory_entry = _directory->getDirectoryEntry(set_index * _associativity + i);
      if (directory_entry->getAddress() == replaced_address)
      {
         replaced_directory_entry = directory_entry;
         _directory->setDirectoryEntry(set_index * _associativity + i, new_directory_entry);
         break;
      }
   }

   LOG_ASSERT_ERROR(replaced_directory_entry, "Could not find address(%#lx) to replace", replaced_address);
   _replaced_directory_entry_list.push_back(replaced_directory_entry);

   if (_enabled)
   {
      // Update Performance Model
      getShmemPerfModel()->incrCurrTime(_directory_access_latency);
      // Update event & dynamic energy counters
      updateCounters();
      // Increment number of evictions
      _total_evictions ++;
      // Increment number of evictions where address is cached (so, leads to a back invalidation)
      DirectoryState::Type replaced_dstate = replaced_directory_entry->getDirectoryBlockInfo()->getDState();
      if (replaced_dstate != DirectoryState::UNCACHED)
         _total_back_invalidations ++;
   }

   return new_directory_entry;
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
   LOG_PRINT_ERROR("Address(%#lx) not found for invalidation", address);
}

void
DirectoryCache::splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index)
{
   IntPtr cache_line_address = address >> _log_cache_line_size;

   tag = cache_line_address;
   set_index = computeSetIndex(address);
}

UInt32
DirectoryCache::computeDirectoryTotalEntries()
{
   // Get dram_directory_total_entries
   UInt32 num_application_tiles = Config::getSingleton()->getApplicationTiles();
   UInt32 total_entries;
   if (_total_entries_str == "auto")
   {
      UInt32 max_L2_cache_size = getMaxL2CacheSize();  // In KB
      UInt32 num_sets = (UInt32) ceil(2.0 * max_L2_cache_size * 1024 * num_application_tiles /
                                      (_cache_line_size * _associativity * _num_directory_slices));
      // Round-off to the nearest power of 2
      num_sets = 1 << ceilLog2(num_sets);
      total_entries = num_sets * _associativity;
   }
   else // (_total_entries_str != "auto")
   {
      total_entries = convertFromString<UInt32>(_total_entries_str);
      LOG_ASSERT_ERROR(total_entries != 0, "Could not parse [dram_directory/total_entries] = %s", _total_entries_str.c_str());
   }

   return total_entries;
}

UInt32
DirectoryCache::getMaxL2CacheSize()  // In KB
{
   UInt32 max_cache_size = 0;
   Config* cfg = Config::getSingleton();
   for (tile_id_t i = 0; i < (tile_id_t) cfg->getApplicationTiles(); i++)
   {
      string type = cfg->getL2CacheType(i);
      
      UInt32 cache_size = 0;
      try
      {
         cache_size = Sim()->getCfg()->getInt("l2_cache/" + type + "/cache_size");
      }
      catch (...)
      {
         LOG_PRINT_ERROR("Could not parse [l2_cache/%s/cache_size] from cfg file", type.c_str());
      }

      if (cache_size > max_cache_size)
         max_cache_size = cache_size;      
   }
   return max_cache_size;
}

UInt64
DirectoryCache::computeDirectoryAccessCycles()
{
   // directory_entry_size is specified in bytes
   // access_time should be computed in cycles
   // access_time is dependent on technology node and frequency
   //   (but these two factors will hopefully cancel each other out to a certain extent)
   
   if (_directory_access_cycles_str == "auto")
   {
      UInt32 directory_size_in_KB = (UInt32) ceil(1.0 * _directory_size / 1024);
      
      if (directory_size_in_KB <= 16)
         return 1;
      else if (directory_size_in_KB <= 32)
         return 2;
      else if (directory_size_in_KB <= 64)
         return 4;
      else if (directory_size_in_KB <= 128)
         return 6;
      else if (directory_size_in_KB <= 256)
         return 8;
      else if (directory_size_in_KB <= 512)
         return 10;
      else if (directory_size_in_KB <= 1024)
         return 13;
      else if (directory_size_in_KB <= 2048)
         return 16;
      else // (directory_size_in_KB > 2048)
         return 20;
   }
   else // (_directory_access_cycles_str != "auto")
   {
      UInt64 directory_access_cycles = convertFromString<UInt64>(_directory_access_cycles_str);
      LOG_ASSERT_ERROR(directory_access_cycles != 0, "Could not parse [dram_directory/access_time] = %s",
            _directory_access_cycles_str.c_str());
      return directory_access_cycles;
   }
}

IntPtr
DirectoryCache::computeSetIndex(IntPtr address)
{
   LOG_PRINT("Computing Set for address(%#lx), _log_cache_line_size(%u), _log_num_sets(%u), _log_num_directory_slices(%u)",
             address, _log_cache_line_size, _log_num_sets, _log_num_directory_slices);

   IntPtr set = 0;
   for (UInt32 i = _log_cache_line_size + _log_num_directory_slices; (i + _log_num_sets) <= (sizeof(IntPtr)*8); i += _log_num_sets)
   {
      IntPtr addr_bits = getBits<IntPtr>(address, i + _log_num_sets, i);
      set = set ^ addr_bits;
   }

   LOG_PRINT("Set(%#x)", (UInt32) set);

   return (UInt32) set;
}

void
DirectoryCache::outputSummary(ostream& out)
{
   printAutogenDirectorySizeAndAccessCycles(out);
   out << "    Total Accesses: " << _total_directory_accesses << endl;
   out << "    Total Evictions: " << _total_evictions << endl;
   out << "    Total Back-Invalidations: " << _total_back_invalidations << endl;

   // Output Power and Area Summaries
   if (Config::getSingleton()->getEnablePowerModeling() || Config::getSingleton()->getEnableAreaModeling())
   {
      // FIXME: Get total cycles from core model
      // _mcpat_cache_interface->computeEnergy(this, 10000);
      // _mcpat_cache_interface->outputSummary(out);
   }

   // Asynchronous communication
   DVFSManager::printAsynchronousMap(out, _module, _asynchronous_map);
}

void
DirectoryCache::dummyOutputSummary(ostream& out, tile_id_t tile_id)
{
   dummyPrintAutogenDirectorySizeAndAccessCycles(out);
   out << "    Total Accesses: " << endl;
   out << "    Total Evictions: " << endl;
   out << "    Total Back-Invalidations: " << endl;

   // Output Power and Area Summaries
   if (Config::getSingleton()->getEnablePowerModeling() || Config::getSingleton()->getEnableAreaModeling())
   {
      // FIXME: Get total cycles from core model
      // _mcpat_cache_interface->computeEnergy(this, 10000);
      // _mcpat_cache_interface->outputSummary(out);
   }
}

void
DirectoryCache::printAutogenDirectorySizeAndAccessCycles(ostream& out)
{
   if (_total_entries_str == "auto")
   {
      out << "    Total Entries [auto-generated]: " << _total_entries << endl;
      UInt32 directory_size_in_KB = (UInt32) ceil(1.0 * _directory_size / 1024);
      out << "    Size (in KB) [auto-generated]: " << directory_size_in_KB << endl;
   }
   if (_directory_access_cycles_str == "auto")
   {
      out << "    Access Time (in clock cycles) [auto-generated]: " << _directory_access_cycles << endl;
   }
}

void
DirectoryCache::dummyPrintAutogenDirectorySizeAndAccessCycles(ostream& out)
{
   if (Sim()->getCfg()->getString("dram_directory/total_entries") == "auto")
   {
      out << "    Total Entries [auto-generated]: " << endl;
      out << "    Size (in KB) [auto-generated]: " << endl;
   }
   if (Sim()->getCfg()->getString("dram_directory/access_time") == "auto")
   {
      out << "    Access Time (in clock cycles) [auto-generated]: " << endl;
   }

}

ShmemPerfModel*
DirectoryCache::getShmemPerfModel()
{
   return _tile->getMemoryManager()->getShmemPerfModel();
}

int
DirectoryCache::getDVFS(double &frequency, double &voltage)
{
   frequency = _frequency;
   voltage = _voltage;
   return 0;
}

int
DirectoryCache::setDVFS(double frequency, voltage_option_t voltage_flag, const Time& curr_time)
{
   int rc = DVFSManager::getVoltage(_voltage, voltage_flag, frequency);
   if (rc==0)
   {
      _frequency = frequency;
      _directory_access_latency = Time(Latency(_directory_access_cycles, _frequency));
      _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), _frequency));
   }

   return rc;
}

Time
DirectoryCache::getSynchronizationDelay(module_t module)
{
   if (!DVFSManager::hasSameDVFSDomain(DIRECTORY, module) && _enabled){
      _asynchronous_map[module] += _synchronization_delay;
      return _synchronization_delay;
   }
   return Time(0);
}
