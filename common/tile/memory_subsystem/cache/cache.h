#pragma once

#include <string>
#include <set>
#include <cassert>
using std::string;
using std::set;

#include "cache_state.h"
#include "cache_perf_model.h"
#include "shmem_perf_model.h"
#include "cache_power_model.h"
#include "cache_area_model.h"
#include "utils.h"
#include "fixed_types.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

// Forwards Decls
class CacheSet;
class CacheLineInfo;

class Cache
{
public:
   // types, constants
   enum AccessType
   {
      INVALID_ACCESS_TYPE = 0,
      MIN_ACCESS_TYPE,
      LOAD = MIN_ACCESS_TYPE,
      STORE,
      MAX_ACCESS_TYPE = STORE,
      NUM_ACCESS_TYPES = MAX_ACCESS_TYPE - MIN_ACCESS_TYPE + 1
   };

   enum ReplacementPolicy
   {
      ROUND_ROBIN = 0,
      LRU,
      NUM_REPLACEMENT_POLICIES
   };

   enum MissType
   {
      INVALID_MISS_TYPE = 0,
      MIN_MISS_TYPE,
      COLD_MISS = MIN_MISS_TYPE,
      CAPACITY_MISS,
      UPGRADE_MISS,
      SHARING_MISS,
      MAX_MISS_TYPE = SHARING_MISS,
      NUM_MISS_TYPES = MAX_MISS_TYPE - MIN_MISS_TYPE + 1
   };

   enum Type
   {
      INVALID_CACHE_TYPE = 0,
      MIN_CACHE_TYPE,
      PR_L1_CACHE = MIN_CACHE_TYPE,
      PR_L2_CACHE,
      MAX_CACHE_TYPE = PR_L2_CACHE,
      NUM_CACHE_TYPES = MAX_CACHE_TYPE - MIN_CACHE_TYPE + 1
   };

   // Constructors/destructors
   Cache(string name, 
         UInt32 cache_size, 
         UInt32 associativity,
         UInt32 cache_line_size,
         string replacement_policy,
         Type cache_type,
         UInt32 access_delay,
         volatile float frequency,
         bool track_miss_types = false);
   ~Cache();

   // Cache operations
   void accessCacheLine(IntPtr address, AccessType access_type, Byte* buf = NULL, UInt32 num_bytes = 0);
   void insertCacheLine(IntPtr inserted_address, CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
                        bool* eviction, IntPtr* evicted_address, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf);
   void getCacheLineInfo(IntPtr address, CacheLineInfo* cache_line_info);
   void setCacheLineInfo(IntPtr address, CacheLineInfo* updated_cache_line_info);
   bool invalidateCacheLine(IntPtr address);

   // Get the tag associated with an address
   IntPtr getTag(IntPtr address) const;
   // Update miss counters - only updated on an access from the core (as opposed from the network)
   MissType updateMissCounters(IntPtr address, bool cache_miss);
   // Get cache line state counters
   void getCacheLineStateCounters(UInt64& total_exclusive_lines, UInt64& total_shared_lines);

   // Parse Miss Type
   static MissType parseMissType(string miss_type);
   
   void enable() { _enabled = true; }
   void disable() { _enabled = false; }
   void reset() {}
   
   virtual void outputSummary(ostream& out);

private:
   // Is enabled?
   bool _enabled;

   // Cache params
   string _name;
   UInt32 _cache_size;
   UInt32 _associativity;
   UInt32 _line_size;
   UInt32 _num_sets;
   UInt32 _log_line_size;
   
   // Cache hit/miss counters
   UInt64 _total_cache_accesses;
   UInt64 _total_cache_misses;
   // Counters for types of misses
   UInt64 _total_cold_misses;
   UInt64 _total_capacity_misses;
   UInt64 _total_upgrade_misses;
   UInt64 _total_sharing_misses;
   // State for tracking type of cache misses
   set<IntPtr> _fetched_address_set;
   set<IntPtr> _evicted_address_set;
   set<IntPtr> _invalidated_address_set;

   // Event counters for tracking tag/data array reads and writes
   UInt64 _tag_array_reads;
   UInt64 _tag_array_writes;
   UInt64 _data_array_reads;
   UInt64 _data_array_writes;

   // Cache line state counters - Number of exclusive and shared lines
   UInt64 _total_exclusive_lines;
   UInt64 _total_shared_lines;

   // Generic Cache Info
   Type _cache_type;
   CacheSet** _sets;

   // Power and Area Models
   CachePowerModel* _power_model;
   CacheAreaModel* _area_model;

   // Track miss types ?
   bool _track_miss_types;
  
   // Utilities
   CacheSet* getSet(IntPtr address) const;
   UInt32 getLineOffset(IntPtr address) const;
   IntPtr getAddressFromTag(IntPtr tag) const;

   // Parse Replacement Policy
   ReplacementPolicy parseReplacementPolicy(string policy);
   
   // Initialize Counters
   // Hit/miss counters
   void initializeMissCounters();
   void initializeMissTypeCounters();
   // Tracking tag/data read/writes
   void initializeTagAndDataArrayCounters();
   // Cache line state counters
   void initializeCacheLineStateCounters();

   // Update counters that record the state of cache lines
   void updateCacheLineStateCounters(CacheState::CState old_cstate, CacheState::CState new_cstate);
   // Update miss type counters
   MissType getMissType(IntPtr address) const;
   void updateMissTypeCounters(IntPtr address, MissType miss_type);
   void clearMissTypeTrackingSets(IntPtr address);
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}
