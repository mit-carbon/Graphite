#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <cassert>

#include "cache_base.h"
#include "cache_set.h"
#include "cache_block_info.h"
#include "utils.h"
#include "hash_map_set.h"
#include "cache_perf_model.h"
#include "shmem_perf_model.h"
#include "log.h"
#include "cache_power_model.h"
#include "cache_area_model.h"

class Cache : public CacheBase
{
   private:
      bool m_enabled;

      // Cache counters
      UInt64 m_total_cache_accesses;
      UInt64 m_total_cache_hits;

      // Event Counters
      UInt64 m_tag_array_reads;
      UInt64 m_tag_array_writes;
      UInt64 m_data_array_reads;
      UInt64 m_data_array_writes;

      // Generic Cache Info
      cache_t m_cache_type;
      CacheSet** m_sets;

      // Power and Area Models
      CachePowerModel* m_power_model;
      CacheAreaModel* m_area_model;
      
   public:

      // constructors/destructors
      Cache(string name, 
            UInt32 cache_size, 
            UInt32 associativity, UInt32 cache_block_size,
            std::string replacement_policy,
            cache_t cache_type,
            UInt32 access_delay,
            volatile float frequency);
      ~Cache();

      bool invalidateSingleLine(IntPtr addr);
      CacheBlockInfo* accessSingleLine(IntPtr addr, 
            access_t access_type, Byte* buff = NULL, UInt32 bytes = 0);
      void insertSingleLine(IntPtr addr, Byte* fill_buff,
            bool* eviction, IntPtr* evict_addr, 
            CacheBlockInfo* evict_block_info, Byte* evict_buff);
      CacheBlockInfo* peekSingleLine(IntPtr addr);

      // Update Cache Counters
      void initializeEventCounters();
      void updateCounters(bool cache_hit);
      void enable() { m_enabled = true; }
      void disable() { m_enabled = false; }
      void reset(); 

      virtual void outputSummary(ostream& out);
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}

#endif /* CACHE_H */
