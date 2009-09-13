#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <cassert>

#include "cache_base.h"
#include "cache_set.h"
#include "cache_block_info.h"
#include "utils.h"
#include "hash_map_set.h"
#include "cache_perf_model_base.h"
#include "shmem_perf_model.h"
#include "log.h"

class Cache : public CacheBase
{
   private:

      // Cache counters
      UInt64 m_num_accesses;
      UInt64 m_num_hits;
      UInt64 m_num_cold_misses;
      UInt64 m_num_capacity_misses;
      UInt64 m_num_upgrade_misses;
      UInt64 m_num_sharing_misses;
      bool m_track_detailed_cache_counters;
      bool m_cache_counters_enabled;
      UInt32 m_cache_access_time;
      HashMapSet<IntPtr>* m_invalidated_set;
      HashMapSet<IntPtr>* m_evicted_set;

      cache_t m_cache_type;
      CacheSet** m_sets;
      ShmemPerfModel* m_shmem_perf_model;

   public:

      // constructors/destructors
      Cache(string name, 
            UInt32 cache_size, 
            UInt32 associativity, UInt32 cache_block_size,
            std::string replacement_policy,
            cache_t cache_type,
            UInt32 cache_access_time,
            bool track_detailed_cache_counters,
            ShmemPerfModel* shmem_perf_model);
      ~Cache();

      bool invalidateSingleLine(IntPtr addr);
      CacheBlockInfo* accessSingleLine(IntPtr addr, 
            access_t access_type, Byte* buff = NULL, UInt32 bytes = 0);
      void insertSingleLine(IntPtr addr, Byte* fill_buff,
            bool* eviction, IntPtr* evict_addr, 
            CacheBlockInfo* evict_block_info, Byte* evict_buff);
      CacheBlockInfo* peekSingleLine(IntPtr addr);
      
      // Update Cache Counters
      void updateCounters(IntPtr addr, CacheState cache_state, access_t access_type);
      void resetCounters();
      void disableCounters();

      UInt64 getNumAccesses() { return m_num_accesses; }
      UInt64 getNumHits() { return m_num_hits; }
      UInt64 getNumColdMisses() { return m_num_cold_misses; }
      UInt64 getNumCapacityMisses() { return m_num_capacity_misses; }
      UInt64 getNumUpgradeMisses() { return m_num_upgrade_misses; }
      UInt64 getNumSharingMisses() { return m_num_sharing_misses; }

      void incrNumAccesses() { m_num_accesses++; }
      void incrNumHits() { m_num_hits++; }
      void incrNumColdMisses() { m_num_cold_misses++; }
      void incrNumCapacityMisses() { m_num_capacity_misses++; }
      void incrNumUpgradeMisses() { m_num_upgrade_misses++; }
      void incrNumSharingMisses() { m_num_sharing_misses++; }
      bool isInvalidated(IntPtr addr)
      {
         return (bool) m_invalidated_set->count(addr);
      }
      bool isEvicted(IntPtr addr)
      {
         return (bool) m_evicted_set->count(addr);
      }

      virtual void outputSummary(ostream& out);
};

template <class T>
UInt32 moduloHashFn(T key, UInt32 hash_fn_param, UInt32 num_buckets)
{
   return (key >> hash_fn_param) % num_buckets;
}

#endif /* CACHE_H */
