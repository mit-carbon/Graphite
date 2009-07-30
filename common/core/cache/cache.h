#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <cassert>

#include "utils.h"
#include "set.h"
#include "cache_set.h"
#include "cache_line.h"
#include "cache_perf_model_base.h"
#include "shmem_perf_model.h"

#define k_KILO 1024
#define k_MEGA (k_KILO*k_KILO)
#define k_GIGA (k_KILO*k_MEGA)

using namespace std;

// type of cache hit/miss counters
typedef UInt64 CacheStats;

typedef enum
{
   STORE_ALLOCATE,
   STORE_NO_ALLOCATE
} StoreAllocation;

// Generic cache base class; no allocate specialization, no cache set specialization
class CacheBase
{
   public:
      // types, constants
      typedef enum
      {
         ACCESS_TYPE_LOAD,
         ACCESS_TYPE_STORE,
         NUM_ACCESS_TYPES
      } AccessType;

      typedef enum
      {
         CACHE_TYPE_ICACHE,
         CACHE_TYPE_DCACHE,
         NUM_CACHE_TYPES
      } CacheType;

   protected:
      //1 counter for hit==true, 1 counter for hit==false
      CacheStats m_access[NUM_ACCESS_TYPES][2];

      // input params
      string m_name;
      UInt32 m_cache_size;
      UInt32 m_blocksize;
      UInt32 m_associativity;
      UInt32 m_num_sets;

      // computed params
      UInt32 m_log_blocksize;

   private:
      CacheStats sumAccess(bool hit) const;

   public:
      // constructors/destructors
      CacheBase(string name);
      virtual ~CacheBase();

      // accessors
      UInt32 getCacheSize() const { return m_cache_size; }
      UInt32 getBlockSize() const { return m_blocksize; }
      UInt32 dCacheLineSize() const { return m_blocksize; }
      UInt32 getNumWays() const { return m_associativity; }
      UInt32 getNumSets() const { return m_num_sets; }

      // stats
      CacheStats getHits(AccessType access_type) const;
      CacheStats getMisses(AccessType access_type) const;
      CacheStats getAccesses(AccessType access_type) const;
      CacheStats getHits() const { return sumAccess(true); }
      CacheStats getMisses() const { return sumAccess(false); }
      CacheStats getAccesses() const { return getHits() + getMisses(); }

      // utilities
      IntPtr tagToAddress(IntPtr tag);
      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index) const;
      void splitAddress(const IntPtr addr, IntPtr& tag, UInt32& set_index, UInt32& block_offset) const;
      
      // Output Summary
      virtual void outputSummary(ostream& out) {}
};

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
      Set<IntPtr>* m_invalidated_set;
      Set<IntPtr>* m_evicted_set;

      CacheSet**  m_sets;
      CachePerfModelBase* m_cache_perf_model;
      ShmemPerfModel* m_shmem_perf_model;

   public:

      // constructors/destructors
      Cache(string name, ShmemPerfModel* shmem_perf_model = NULL);
      ~Cache();

      bool invalidateSingleLine(IntPtr addr);
      CacheBlockInfo* accessSingleLine(IntPtr addr, 
            AccessType access_type, Byte* buff = NULL, UInt32 bytes = 0);
      void insertSingleLine(IntPtr addr, Byte* fill_buff,
            bool* eviction, IntPtr* evict_addr, 
            CacheBlockInfo* evict_block_info, Byte* evict_buff);
      CacheBlockInfo* peekSingleLine(IntPtr addr);
      
      // Update Cache Counters
      void updateCounters(IntPtr addr, CacheState cache_state, AccessType access_type);
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
