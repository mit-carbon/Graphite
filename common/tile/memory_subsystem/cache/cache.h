#pragma once

#include <string>
#include <set>
#include <cassert>
using std::string;
using std::set;

#include "core.h"
#include "cache_state.h"
#include "cache_perf_model.h"
#include "shmem_perf_model.h"
#include "cache_power_model.h"
#include "cache_area_model.h"
#include "utils.h"
#include "fixed_types.h"
#include "caching_protocol_type.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

// Forwards Decls
class CacheSet;
class CacheLineInfo;
class CacheReplacementPolicy;

class Cache
{
public:
   // types, constants
   enum AccessType
   {
      LOAD = 0,
      STORE
   };

   enum MissType
   {
      COLD_MISS = 0,
      CAPACITY_MISS,
      SHARING_MISS,
      NUM_MISS_TYPES,
      INVALID_MISS_TYPE
   };

   enum CacheCategory
   {
      INSTRUCTION_CACHE,
      DATA_CACHE,
      UNIFIED_CACHE
   };

   enum WritePolicy
   {
      UNDEFINED_WRITE_POLICY = 0,
      WRITE_THROUGH,
      WRITE_BACK
   };

   // Constructors/destructors
   Cache(string name, 
         CachingProtocolType caching_protocol_type,
         CacheCategory cache_category,
         SInt32 cache_level,
         WritePolicy write_policy,
         UInt32 cache_size, 
         UInt32 associativity,
         UInt32 line_size,
         CacheReplacementPolicy* replacement_policy,
         UInt32 access_delay,
         float frequency,
         bool track_miss_types = false);
   ~Cache();

   // Cache operations
   void accessCacheLine(IntPtr address, AccessType access_type, Byte* buf = NULL, UInt32 num_bytes = 0);
   void insertCacheLine(IntPtr inserted_address, CacheLineInfo* inserted_cache_line_info, Byte* fill_buf,
                        bool* eviction, IntPtr* evicted_address, CacheLineInfo* evicted_cache_line_info, Byte* writeback_buf);
   void getCacheLineInfo(IntPtr address, CacheLineInfo* cache_line_info);
   void setCacheLineInfo(IntPtr address, CacheLineInfo* updated_cache_line_info);

   // Get the tag associated with an address
   IntPtr getTag(IntPtr address) const;
   // Get the number of sets in the cache
   UInt32 getNumSets() const
   { return _num_sets; }
   // Get the associativity of the L2 cache
   UInt32 getAssociativity() const
   { return _associativity; }
   // Update miss counters - only updated on an access from the core (as opposed from the network)
   MissType updateMissCounters(IntPtr address, Core::mem_op_t mem_op_type, bool cache_miss);
   // Get cache line state counters
   void getCacheLineStateCounters(vector<UInt64>& cache_line_state_counters) const;

   // Parse Miss Type
   static MissType parseMissType(string miss_type);
   
   void enable()     { _enabled = true; }
   void disable()    { _enabled = false; }
   void reset()      {}
   
   virtual void outputSummary(ostream& out);

private:
   // Is enabled?
   bool _enabled;

   // Generic Cache Info
   string _name;
   CacheCategory _cache_category;
   WritePolicy _write_policy;
   CacheSet** _sets;

   // Cache params
   UInt32 _cache_size;
   UInt32 _associativity;
   UInt32 _line_size;
   UInt32 _num_sets;
   UInt32 _log_line_size;
   
   // Cache hit/miss counters
   UInt64 _total_cache_accesses;
   UInt64 _total_cache_misses;
   UInt64 _total_read_accesses;
   UInt64 _total_read_misses;
   UInt64 _total_write_accesses;
   UInt64 _total_write_misses;
   
   // Counters for types of misses
   UInt64 _total_cold_misses;
   UInt64 _total_capacity_misses;
   UInt64 _total_sharing_misses;
   // State for tracking type of cache misses
   set<IntPtr> _fetched_address_set;
   set<IntPtr> _evicted_address_set;
   set<IntPtr> _invalidated_address_set;

   // Evictions
   UInt64 _total_evictions;
   UInt64 _total_dirty_evictions;
   
   // Event counters for tracking tag/data array reads and writes
   UInt64 _tag_array_reads;
   UInt64 _tag_array_writes;
   UInt64 _data_array_reads;
   UInt64 _data_array_writes;

   // Cache line state counters - Number of exclusive and shared lines
   vector<UInt64> _cache_line_state_counters;

   // Power and Area Models
   CachePowerModel* _power_model;
   CacheAreaModel* _area_model;

   // Track miss types ?
   bool _track_miss_types;
  
   // Utilities
   CacheSet* getSet(IntPtr address) const;
   UInt32 getLineOffset(IntPtr address) const;
   IntPtr getAddressFromTag(IntPtr tag) const;

   // Initialize Counters
   // Hit/miss counters
   void initializeMissCounters();
   void initializeMissTypeCounters();
   // Initialize eviction counters
   void initializeEvictionCounters();
   // Tracking tag/data read/writes
   void initializeTagAndDataArrayCounters();
   // Cache line state counters
   void initializeCacheLineStateCounters();

   // Get cache line info
   CacheLineInfo* getCacheLineInfo(IntPtr address);

   // Update miss type counters
   MissType getMissType(IntPtr address) const;
   void updateMissTypeCounters(IntPtr address, MissType miss_type);
   void clearMissTypeTrackingSets(IntPtr address);
   
   // Update counters that record the state of cache lines
   void updateCacheLineStateCounters(CacheState::Type old_cstate, CacheState::Type new_cstate);
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}
