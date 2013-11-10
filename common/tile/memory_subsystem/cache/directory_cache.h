#pragma once

#include <string>
#include <map>
using std::string;
using std::map;
using std::ostream;

#include "tile.h"
#include "directory.h"
#include "shmem_perf_model.h"
#include "directory_entry.h"
#include "directory_type.h"
#include "caching_protocol_type.h"
#include "dvfs.h"
#include "dvfs_manager.h"

class McPATCacheInterface;

class DirectoryCache
{
public:
   DirectoryCache(Tile* tile,
                  CachingProtocolType caching_protocol_type,
                  string directory_type_str,
                  string total_entries_str,
                  UInt32 associativity,
                  UInt32 cache_line_size,
                  UInt32 max_hw_sharers,
                  UInt32 max_num_sharers,
                  UInt32 num_directory_slices,
                  string directory_access_time_str,
                  ShmemPerfModel* shmem_perf_model);
   ~DirectoryCache();

   Directory* getDirectory() { return _directory; }
   DirectoryEntry* getDirectoryEntry(IntPtr address);
   DirectoryEntry* replaceDirectoryEntry(IntPtr replaced_address, IntPtr address);
   void invalidateDirectoryEntry(IntPtr address);
   void getReplacementCandidates(IntPtr address, vector<DirectoryEntry*>& replacement_candidate_list);

   void outputSummary(ostream& os);
   static void dummyOutputSummary(ostream& os, tile_id_t tile_id);

   void enable() { _enabled = true; }
   void disable() { _enabled = false; }

   int getDVFS(double &frequency, double &voltage);
   int setDVFS(double frequency, voltage_option_t voltage_flag, const Time& curr_time);
   Time getSynchronizationDelay(module_t module);

private:
   Tile* _tile;
   Directory* _directory;
   vector<DirectoryEntry*> _replaced_directory_entry_list;
   
   map<IntPtr,UInt64> _address_map;
   vector<map<IntPtr,UInt64> > _set_specific_address_map;
   map<IntPtr,UInt64> _replaced_address_map;
   vector<UInt64> _set_replacement_histogram;

   CachingProtocolType _caching_protocol_type;
   DirectoryType _directory_type;
   UInt32 _max_hw_sharers;
   UInt32 _max_num_sharers;

   string _total_entries_str;
   UInt32 _total_entries;
   UInt32 _associativity;
   UInt32 _directory_size; // In bytes

   UInt32 _num_sets;
   UInt32 _cache_line_size;
   UInt32 _num_directory_slices;

   UInt32 _log_num_sets;
   UInt32 _log_cache_line_size;
   UInt32 _log_num_application_tiles;
   UInt32 _log_num_directory_slices;

   string _directory_access_cycles_str;
   UInt64 _directory_access_cycles;
   Time _directory_access_latency;
   Time _synchronization_delay;

   // Dram Directory Cache Power and Area Models (through McPAT)
   McPATCacheInterface* _mcpat_cache_interface;

   // Counters
   UInt64 _total_directory_accesses;
   UInt64 _total_evictions;
   UInt64 _total_back_invalidations;

   bool _enabled;

   // Auto(-matically) determine total number of entries in the directory
   UInt32 computeDirectoryTotalEntries();
   // Get the max L2 cache size (in KB)
   UInt32 getMaxL2CacheSize();
   // Auto(-matically) determine directory access time
   UInt64 computeDirectoryAccessCycles();

   void initializeEventCounters();
   void splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index);

   void updateCounters();
   IntPtr computeSetIndex(IntPtr address);
  
   // Output auto-generated directory size and access time
   void printAutogenDirectorySizeAndAccessCycles(ostream& out);
   static void dummyPrintAutogenDirectorySizeAndAccessCycles(ostream& out);

   ShmemPerfModel* getShmemPerfModel();

   double _frequency;
   double _voltage;
   module_t _module;
   DVFSManager::AsynchronousMap _asynchronous_map;
   ShmemPerfModel* _shmem_perf_model;
};
