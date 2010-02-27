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

class Cache : public CacheBase
{
   private:
      bool m_enabled;

      // Cache counters
      UInt64 m_num_accesses;
      UInt64 m_num_hits;

      // Generic Cache Info
      cache_t m_cache_type;
      CacheSet** m_sets;
      
      // Perf Modelling
      ShmemPerfModel* m_shmem_perf_model;
      CachePerfModel* m_cache_perf_model;

   public:

      // constructors/destructors
      Cache(string name, 
            UInt32 cache_size, 
            UInt32 associativity, UInt32 cache_block_size,
            std::string replacement_policy,
            cache_t cache_type,
            UInt32 cache_data_access_time,
            UInt32 cache_tags_access_time,
            std::string cache_perf_model_type,
            ShmemPerfModel* shmem_perf_model);
      ~Cache();

      bool invalidateSingleLine(IntPtr addr);
      CacheBlockInfo* accessSingleLine(IntPtr addr, 
            access_t access_type, Byte* buff = NULL, UInt32 bytes = 0);
      void insertSingleLine(IntPtr addr, Byte* fill_buff,
            bool* eviction, IntPtr* evict_addr, 
            CacheBlockInfo* evict_block_info, Byte* evict_buff);
      CacheBlockInfo* peekSingleLine(IntPtr addr);

      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }
      CachePerfModel* getCachePerfModel() { return m_cache_perf_model; }
      
      // Update Cache Counters
      void updateCounters(bool cache_hit);

      void enable() { m_enabled = true; getCachePerfModel()->enable(); }
      void disable() { m_enabled = false; getCachePerfModel()->disable(); }

      virtual void outputSummary(ostream& out);
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}

#endif /* CACHE_H */
