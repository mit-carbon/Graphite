#pragma once

#include <string>
#include <map>
#include <vector>
using std::string;
using std::vector;
using std::map;
using std::ostream;

#include "tile.h"
#include "directory.h"
#include "shmem_perf_model.h"
#include "cache_power_model.h"
#include "cache_area_model.h"
#include "directory_entry.h"
#include "directory_type.h"
#include "caching_protocol_type.h"

class DirectoryCache
{
public:
   DirectoryCache(Tile* tile,
                  CachingProtocolType caching_protocol_type,
                  string directory_type_str,
                  UInt32 total_entries,
                  UInt32 associativity,
                  UInt32 cache_line_size,
                  UInt32 max_hw_sharers,
                  UInt32 max_num_sharers,
                  UInt32 num_directories,
                  UInt64 directory_access_delay_in_clock_cycles);
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
   UInt32 _total_entries;
   UInt32 _associativity;

   UInt32 _num_sets;
   UInt32 _cache_line_size;
   UInt32 _num_directories;

   UInt32 _log_num_sets;
   UInt32 _log_cache_line_size;
   UInt32 _log_num_application_tiles;
   UInt32 _log_num_directories;

   UInt32 _log_stack_size;

   UInt64 _directory_access_delay_in_clock_cycles;

   // Dram Directory Cache Power and Area Models
   CachePowerModel* _power_model;
   CacheAreaModel* _area_model;

   // Counters
   UInt64 _total_directory_accesses;
   // Tag & Data read/write counters
   UInt64 _tag_array_reads;
   UInt64 _tag_array_writes;
   UInt64 _data_array_reads;
   UInt64 _data_array_writes;

   bool _enabled;

   ShmemPerfModel* getShmemPerfModel();

   void initializeParameters();
   void initializeEventCounters();
   void splitAddress(IntPtr address, IntPtr& tag, UInt32& set_index);

   void updateCounters();
   IntPtr computeSetIndex(IntPtr address);
};
