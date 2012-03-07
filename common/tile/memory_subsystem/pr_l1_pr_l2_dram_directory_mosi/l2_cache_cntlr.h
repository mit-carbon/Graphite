#pragma once

// Forward declarations
namespace PrL1PrL2DramDirectoryMOSI
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include <map>
using std::map;

#include "cache.h"
#include "pr_l2_cache_line_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "shmem_req.h"
#include "mem_component.h"
#include "semaphore.h"
#include "lock.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "aggregate_cache_line_utilization.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class L2CacheCntlr
   {
   public:
      L2CacheCntlr(MemoryManager* memory_manager,
                   L1CacheCntlr* L1_cache_cntlr,
                   AddressHomeLookup* dram_directory_home_lookup,
                   Semaphore* user_thread_sem,
                   Semaphore* network_thread_sem,
                   UInt32 cache_line_size,
                   UInt32 L2_cache_size,
                   UInt32 L2_cache_associativity,
                   string L2_cache_replacement_policy,
                   UInt32 L2_cache_access_delay,
                   bool L2_cache_track_miss_types,
                   volatile float frequency);
      ~L2CacheCntlr();

      Cache* getL2Cache() { return _L2_cache; }

      // Handle Request from L1 Cache - This is done for better simulator performance
      pair<bool,Cache::MissType> processShmemRequestFromL1Cache(MemComponent::component_t req_mem_component, Core::mem_op_t mem_op_type, IntPtr address);
      // Write-through Cache. Hence needs to be written by the APP thread
      void writeCacheLine(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length);
      void assertCacheLineWritable(IntPtr address);

      // Handle message from L1 Cache
      void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
      // Handle message from Dram Dir
      void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      // Output summary
      void outputSummary(ostream& out);
      // Acquiring and Releasing Locks
      void acquireLock();
      void releaseLock();

      void enable() { _enabled = true; }
      void disable() { _enabled = false; }

   private:
      // Data Members
      MemoryManager* _memory_manager;
      Cache* _L2_cache;
      L1CacheCntlr* _L1_cache_cntlr;
      AddressHomeLookup* _dram_directory_home_lookup;

      // Outstanding ShmemReq info
      ShmemMsg _outstanding_shmem_msg;
      UInt64 _outstanding_shmem_msg_time;

      // Synchronization
      Lock _L2_cache_lock;
      Semaphore* _user_thread_sem;
      Semaphore* _network_thread_sem;

      // Is enabled?
      bool _enabled;

      // Measure cache line utilization
      UInt64 _total_evictions;
      UInt64 _total_invalidations;
      static const UInt32 MAX_TRACKED_LOCAL_UTILIZATION = 31;
      UInt64 _total_evictions_by_utilization[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      UInt64 _total_invalidations_by_utilization[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      AggregateCacheLineUtilization _utilization_counters_for_eviction[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      AggregateCacheLineUtilization _utilization_counters_for_invalidation[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      UInt64 _modified_state_count_during_eviction[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      UInt64 _shared_state_count_during_eviction[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      UInt64 _modified_state_count_during_invalidation[MAX_TRACKED_LOCAL_UTILIZATION + 1];
      UInt64 _shared_state_count_during_invalidation[MAX_TRACKED_LOCAL_UTILIZATION + 1];

      // Measure clean/dirty evictions
      UInt64 _total_dirty_evictions_exreq;
      UInt64 _total_clean_evictions_exreq;
      UInt64 _total_dirty_evictions_shreq;
      UInt64 _total_clean_evictions_shreq;

      // L2 cache operations
      void getCacheLineInfo(IntPtr address, PrL2CacheLineInfo* L2_cache_line_info);
      void setCacheLineInfo(IntPtr address, PrL2CacheLineInfo* L2_cache_line_info);
      void invalidateCacheLine(IntPtr address);
      void readCacheLine(IntPtr address, Byte* data_buf);
      void insertCacheLine(IntPtr address, CacheState::CState cstate, Byte* fill_buf, MemComponent::component_t mem_component);

      // L1 cache operations
      void setCacheLineStateInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::CState cstate);
      void invalidateCacheLineInL1(MemComponent::component_t mem_component, IntPtr address);
      void insertCacheLineInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::CState cstate, Byte* fill_buf);
      void updateAggregateCacheLineUtilizationFromL1(AggregateCacheLineUtilization& aggregate_utilization,
                                                     MemComponent::component_t mem_component, IntPtr address);

      // Insert cache line in hierarchy
      void insertCacheLineInHierarchy(IntPtr address, CacheState::CState cstate, Byte* fill_buf);

      // Process Request from L1 Cache
      // Check if msg from L1 ends in the L2 cache
      pair<bool,Cache::MissType> operationPermissibleinL2Cache(Core::mem_op_t mem_op_type, IntPtr address, CacheState::CState cstate);

      // Process Request from Dram Dir
      void processExRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processShRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processUpgradeRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processInvFlushCombinedReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);

      void processBufferedShmemReqFromDramDirectory(void);

      // Utilities
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager()   { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();

      // Wake up User Thread
      void wakeUpUserThread();
      // Wait for User Thread
      void waitForUserThread();

      // Dram Directory Home Lookup
      tile_id_t getHome(IntPtr address) { return _dram_directory_home_lookup->getHome(address); }

      MemComponent::component_t acquireL1CacheLock(ShmemMsg::msg_t msg_type, IntPtr address);

      // Cache line utilization
      void initializeCounters();
      void updateEvictionCounters(CacheState::CState inserted_cstate, CacheState::CState evicted_cstate);
      void updateUtilizationCountersOnEviction(AggregateCacheLineUtilization& aggregate_utilization, CacheState::CState evicted_cstate);
      void updateUtilizationCountersOnInvalidation(AggregateCacheLineUtilization& aggregate_utilization, CacheState::CState invalidated_cstate);
   };

}
