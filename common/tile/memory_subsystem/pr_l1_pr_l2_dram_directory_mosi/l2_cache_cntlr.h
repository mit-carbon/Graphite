#pragma once

// Forward declarations
namespace PrL1PrL2DramDirectoryMOSI
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include <map>
using std::map;

#include "common_defines.h"
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

#ifdef TRACK_UTILIZATION_COUNTERS
#include "aggregate_cache_line_utilization.h"
#endif

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
      pair<bool,Cache::MissType> processShmemRequestFromL1Cache(MemComponent::Type req_mem_component, Core::mem_op_t mem_op_type, IntPtr address);
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
      enum CacheOperationType
      {
         CACHE_EVICTION,
         CACHE_INVALIDATION,
         NUM_CACHE_OPERATION_TYPES
      };

      // Data Members
      MemoryManager* _memory_manager;
      Cache* _L2_cache;
      L1CacheCntlr* _L1_cache_cntlr;
      AddressHomeLookup* _dram_directory_home_lookup;

      // Outstanding ShmemReq info
      ShmemMsg _outstanding_shmem_msg;
      UInt64 _outstanding_shmem_Typeime;

      // Synchronization
      Lock _L2_cache_lock;
      Semaphore* _user_thread_sem;
      Semaphore* _network_thread_sem;

      // Is enabled?
      bool _enabled;

      // Utilization counters
      UInt64 _total_cache_operations[NUM_CACHE_OPERATION_TYPES];
      UInt64 _total_cache_operations_by_utilization[NUM_CACHE_OPERATION_TYPES][MAX_TRACKED_UTILIZATION + 1];
      AggregateCacheLineUtilization _utilization_counters[NUM_CACHE_OPERATION_TYPES][MAX_TRACKED_UTILIZATION + 1];
      UInt64 _modified_state_count_by_utilization[NUM_CACHE_OPERATION_TYPES][MAX_TRACKED_UTILIZATION + 1];
      UInt64 _shared_state_count_by_utilization[NUM_CACHE_OPERATION_TYPES][MAX_TRACKED_UTILIZATION + 1];

      // Lifetime counters
      AggregateCacheLineLifetime _lifetime_counters[NUM_CACHE_OPERATION_TYPES][MAX_TRACKED_UTILIZATION + 1];

      // Eviction Counters
      // Measure clean/dirty evictions
      UInt64 _total_dirty_evictions_exreq;
      UInt64 _total_clean_evictions_exreq;
      UInt64 _total_dirty_evictions_shreq;
      UInt64 _total_clean_evictions_shreq;

      // L2 cache operations
      void getCacheLineInfo(IntPtr address, PrL2CacheLineInfo* L2_cache_line_info);
      void setCacheLineInfo(IntPtr address, PrL2CacheLineInfo* L2_cache_line_info);
      void invalidateCacheLine(IntPtr address, CacheLineUtilization& net_cache_line_utilization, UInt64 time);
      void readCacheLine(IntPtr address, Byte* data_buf);
      void insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component);

      // L1 cache operations
      void setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
      void invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address, CacheLineUtilization& net_cache_line_utilization, UInt64 time);
      void insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf);

      // Insert cache line in hierarchy
      void insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, Byte* fill_buf);

      // Process Request from L1 Cache
      // Check if msg from L1 ends in the L2 cache
      pair<bool,Cache::MissType> operationPermissibleinL2Cache(Core::mem_op_t mem_op_type, IntPtr address, CacheState::Type cstate);

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
      UInt64 getTime();

      // Wake up User Thread
      void wakeUpUserThread();
      // Wait for User Thread
      void waitForUserThread();

      // Dram Directory Home Lookup
      tile_id_t getHome(IntPtr address) { return _dram_directory_home_lookup->getHome(address); }

      MemComponent::Type acquireL1CacheLock(ShmemMsg::Type msg_type, IntPtr address);

      // Cache line utilization
      void initializeUtilizationCounters();
      void initializeEvictionCounters();
      void updateUtilizationCounters(CacheOperationType operation, AggregateCacheLineUtilization& aggregate_utilization, CacheState::Type cstate);
      void updateLifetimeCounters(CacheOperationType operation, AggregateCacheLineLifetime& aggregate_lifetime, UInt64 cache_line_utilization);
      void updateLifetimeCounters(CacheOperationType operation, MemComponent::Type mem_component, UInt64 cache_line_lifetime, UInt64 cache_line_utilization);
      void updateEvictionCounters(CacheState::Type inserted_cstate, CacheState::Type evicted_cstate);
   };

}
