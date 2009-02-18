// Jonathan Eastep (eastep@mit.edu)
// 04.09.08
//
// This file implements a dynamically adaptive cache. The Organic Cache
// (OCache) consists of an instruction L1 and data L1 which share a pool
// of cache banks. Each bank implements a "way" in terms of associativity,
// so there are n ways split arbitrarily between the icache and dcache.
// Note: the number of physical sets in the OCache is fixed, so it adjusts
// simultaneously the cache sizes when it reapportions the banks.
// The OCache monitors cache access statistics to automatically manage
// bank reapportioning.


#ifndef OCACHE_H
#define OCACHE_H

#include <iostream>
#include <fstream>
#include <sstream>

#include "cache.h"
#include "pin.H"
#include "pin_profile.H"
#include "memory_manager.h"

class Core;

class OCache
{
   public:
      // typedefs
      typedef enum
      {
         k_COUNTER_MISS = 0,
         k_COUNTER_HIT = 1,
         k_COUNTER_NUM
      } Counter;

      typedef  COUNTER_ARRAY<UInt64, k_COUNTER_NUM> CounterArray;

      // Constructor
      OCache(std::string name, Core *core);

      // These are just wrappers around the Cache class equivalents for the OCache dcache field
      bool dCacheInvalidateLine(IntPtr d_addr) { return m_dl1->invalidateLine(d_addr); }
      UInt32 dCacheSize() { return m_dl1->getCacheSize(); }
      UInt32 dCacheLineSize() { return m_dl1->getLineSize(); }
      UInt32 dCacheAssociativity() { return m_dl1->getNumWays(); }
      UInt32 dCacheGetSetPtr(UInt32 set_index) { return m_dl1->getSetPtr(set_index); }
      void   dCacheSetSetPtr(UInt32 set_index, UInt32 value) { m_dl1->setSetPtr(set_index, value); }
      string dCacheStatsLong(string prefix, CacheBase::CacheType type)
      { return m_dl1->statsLong(prefix,type); }

      // These are just wrappers around the Cache class equivalents for the OCache icache field
      bool iCacheInvalidateLine(IntPtr i_addr) { return m_il1->invalidateLine(i_addr); }
      UInt32 iCacheSize() { return m_il1->getCacheSize(); }
      UInt32 iCacheLineSize() { return m_il1->getLineSize(); }
      UInt32 iCacheAssociativity() { return m_il1->getNumWays(); }
      UInt32 iCacheGetSetPtr(UInt32 set_index) { return m_il1->getSetPtr(set_index); }
      void   iCacheSetSetPtr(UInt32 set_index, UInt32 value) { m_il1->setSetPtr(set_index, value); }
      string iCacheStatsLong(string prefix, CacheBase::CacheType type) { return m_il1->statsLong(prefix,type); }

      string statsLong();

      bool runDCacheModel(CacheBase::AccessType operation, IntPtr d_addr, char* data_buffer, UInt32 data_size);

      // These functions provide the public interface to accessing the caches
      pair<bool, CacheTag*> runICacheLoadModel(IntPtr i_addr, UInt32 size);
      pair<bool, CacheTag*> runDCacheLoadModel(IntPtr d_addr, UInt32 size);
      pair<bool, CacheTag*> runDCacheStoreModel(IntPtr d_addr, UInt32 size);
      // These are side-effect free (don't update stats, don't cause eviction, etc.)
      pair<bool, CacheTag*> runICachePeekModel(IntPtr i_addr);
      pair<bool, CacheTag*> runDCachePeekModel(IntPtr d_addr);

      /********** CELIO WAS HERE **************/
      pair<bool, CacheTag*> accessSingleLine(IntPtr addr, CacheBase::AccessType access_type,
                                             bool* fail_need_fill = NULL, char* fill_buff = NULL,
                                             char* buff = NULL, UInt32 bytes = 0,
                                             bool* eviction = NULL, IntPtr* evict_addr = NULL, char* evict_buff = NULL)
      {
         //TODO !!!! I don't have direct access to m_dl1 from the memoryManager, and until i have better stubs to talk to the m_dl1,
         //I'm just gonna pass accessSingleLine calls straight through.
         return m_dl1->accessSingleLine(addr, access_type, fail_need_fill, fill_buff, buff, bytes, eviction, evict_addr, evict_buff);
      }

      bool invalidateLine(IntPtr addr)
      {
         return m_dl1->invalidateLine(addr);
      }
      /****************************************/

      // This function is called at the end of simulation
      void outputSummary(ostream& out);

   private:
      typedef Cache<CACHE_SET::RoundRobin<16, 128>, 1024, 64, CACHE_ALLOC::k_STORE_ALLOCATE> RRSACache;

      RRSACache   *m_dl1;
      RRSACache   *m_il1;

      UInt32      m_cache_size;
      UInt32      m_line_size;
      UInt32      m_associativity;

      UInt32      m_mutation_interval;
      UInt64      m_dcache_total_accesses;
      UInt32      m_dcacheAccesses;
      UInt64      m_dcache_total_misses;
      UInt32      m_dcache_misses;
      UInt64      m_icache_total_accesses;
      UInt32      m_icache_accesses;
      UInt64      m_icache_total_misses;
      UInt32      m_icache_misses;
      UInt64      m_total_resize_evictions;

      UInt32      m_last_dcache_misses;
      UInt32      m_last_icache_misses;

      string      m_name;
      Core *      m_core;

   private:

      void resetIntervalCounters()
      {
         m_dcache_misses = 0;
         m_dcacheAccesses = 0;
         m_icache_misses = 0;
         m_icache_accesses = 0;
      }

      // These functions define the concurrent evolution heuristic of the dcache and icache
      void evolveNaive();
      void evolveDataIntensive();

      // This function increments an interval counter (triggered by some cache event)
      // When the counter reaches a threshold, it fires an evolution function
      void mutationRuntime();

      // These functions access the dcache and icache
      // You can access either a single line or multi lines
      // Fast accesses skip adding the access to the profiler
      pair<bool, CacheTag*> dCacheLoadSingle(IntPtr addr, UInt32 inst_id);
      pair<bool, CacheTag*> dCacheLoadSingleFast(IntPtr addr);
      pair<bool, CacheTag*> dCacheLoadMultiFast(IntPtr addr, UInt32 size);
      pair<bool, CacheTag*> dCacheStoreSingle(IntPtr addr, UInt32 inst_id);
      pair<bool, CacheTag*> dCacheStoreSingleFast(IntPtr addr);
      pair<bool, CacheTag*> dCacheStoreMultiFast(IntPtr addr, UInt32 size);
      pair<bool, CacheTag*> iCacheLoadSingle(IntPtr addr, UInt32 inst_id);
      pair<bool, CacheTag*> iCacheLoadSingleFast(IntPtr addr);
      pair<bool, CacheTag*> iCacheLoadMultiFast(IntPtr addr, UInt32 size);

      // holds the counters with misses and hits
      // conceptually this is an array indexed by instruction address
      COMPRESSOR_COUNTER<IntPtr, UInt32, CounterArray> dcache_profile;
      COMPRESSOR_COUNTER<IntPtr, UInt32, CounterArray> icache_profile;
};

#endif
