#pragma once

#include <string>
using std::string;

// Forward declaration
namespace PrL1PrL2DramDirectoryMOSI
{
   class L2CacheCntlr;
   class MemoryManager;
}

#include "tile.h"
#include "cache.h"
#include "cache_line_info.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "lock.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"

#include "common_defines.h"
#ifdef TRACK_DETAILED_CACHE_COUNTERS
#include "aggregate_cache_line_utilization.h"
#include "aggregate_cache_line_lifetime.h"
#endif

namespace PrL1PrL2DramDirectoryMOSI
{
   class L1CacheCntlr
   {
   public:
      L1CacheCntlr(MemoryManager* memory_manager,
                   Semaphore* app_thread_sem,
                   Semaphore* sim_thread_sem,
                   UInt32 cache_line_size,
                   UInt32 L1_icache_size,
                   UInt32 L1_icache_associativity,
                   string L1_icache_replacement_policy,
                   UInt32 L1_icache_access_delay,
                   bool L1_icache_track_miss_types,
                   UInt32 L1_dcache_size,
                   UInt32 L1_dcache_associativity,
                   string L1_dcache_replacement_policy,
                   UInt32 L1_dcache_access_delay,
                   bool L1_dcache_track_miss_types,
                   float frequency);
      ~L1CacheCntlr();

      Cache* getL1ICache() { return _L1_icache; }
      Cache* getL1DCache() { return _L1_dcache; }

      void setL2CacheCntlr(L2CacheCntlr* L2_cache_cntlr);

      bool processMemOpFromTile(MemComponent::Type mem_component,
            Core::lock_signal_t lock_signal,
            Core::mem_op_t mem_op_type, 
            IntPtr ca_address, UInt32 offset,
            Byte* data_buf, UInt32 data_length,
            bool modeled);

      void insertCacheLine(MemComponent::Type mem_component,
                           IntPtr address, CacheState::Type cstate, Byte* data_buf,
                           bool* eviction_ptr, PrL1CacheLineInfo* evicted_cache_line_info, IntPtr* evict_address_ptr,
                           UInt64 curr_time);

      CacheState::Type getCacheLineState(MemComponent::Type mem_component, IntPtr address);
      void setCacheLineState(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
      void invalidateCacheLine(MemComponent::Type mem_component, IntPtr address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                              , CacheLineUtilization& cache_line_utilization, UInt64 curr_time
#endif
                              );

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Cache line utilization & lifetime
      void updateAggregateCacheLineUtilization(AggregateCacheLineUtilization& aggregate_utilization,
                                               MemComponent::Type mem_component, IntPtr address);
      void updateAggregateCacheLineLifetime(AggregateCacheLineLifetime& aggregate_lifetime,
                                            MemComponent::Type mem_component, IntPtr address, UInt64 curr_time);
#endif

      void acquireLock(MemComponent::Type mem_component);
      void releaseLock(MemComponent::Type mem_component);
   
   private:
      MemoryManager* _memory_manager;
      Cache* _L1_icache;
      Cache* _L1_dcache;
      L2CacheCntlr* _L2_cache_cntlr;

      Lock _L1_icache_lock;
      Lock _L1_dcache_lock;
      Semaphore* _app_thread_sem;
      Semaphore* _sim_thread_sem;

      void accessCache(MemComponent::Type mem_component,
            Core::mem_op_t mem_op_type, 
            IntPtr ca_address, UInt32 offset,
            Byte* data_buf, UInt32 data_length);
      bool operationPermissibleinL1Cache(MemComponent::Type mem_component, 
            IntPtr address, Core::mem_op_t mem_op_type,
            UInt32 access_num);

      Cache* getL1Cache(MemComponent::Type mem_component);
      ShmemMsg::Type getShmemMsgType(Core::mem_op_t mem_op_type);

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Cache line utilization & lifetime
      CacheLineUtilization getCacheLineUtilization(MemComponent::Type mem_component, IntPtr address);
      UInt64 getCacheLineLifetime(MemComponent::Type mem_component, IntPtr address, UInt64 curr_time);
#endif
      
      // Utilities
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager()   { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();

      // Wait for Sim Thread
      void waitForSimThread();
      // Wake up Sim Thread
      void wakeUpSimThread();
   };
}
