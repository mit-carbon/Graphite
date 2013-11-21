#pragma once

// Forward declarations
namespace PrL1PrL2DramDirectoryMOSI
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "cache_line_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "shmem_req.h"
#include "mem_component.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "cache_replacement_policy.h"
#include "cache_hash_fn.h"
#include "time_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class L2CacheCntlr
   {
   public:
      L2CacheCntlr(MemoryManager* memory_manager,
                   L1CacheCntlr* L1_cache_cntlr,
                   AddressHomeLookup* dram_directory_home_lookup,
                   UInt32 cache_line_size,
                   UInt32 L2_cache_size,
                   UInt32 L2_cache_associativity,
                   UInt32 L2_cache_num_banks,
                   string L2_cache_replacement_policy,
                   UInt32 L2_cache_data_access_cycles,
                   UInt32 L2_cache_tags_access_cycles,
                   string L2_cache_perf_model_type,
                   bool L2_cache_track_miss_types);
      ~L2CacheCntlr();

      Cache* getL2Cache() { return _L2_cache; }

      // Handle Request from L1 Cache - This is done for better simulator performance
      pair<bool,Cache::MissType> processShmemRequestFromL1Cache(MemComponent::Type req_mem_component, Core::mem_op_t mem_op_type, IntPtr address);
      // Write-through Cache. Hence needs to be written by the APP thread
      void writeCacheLine(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length);

      // Handle message from L1 Cache
      void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
      // Handle message from Dram Dir
      void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      // Output summary
      void outputSummary(ostream& out);

      void enable() { _enabled = true; }
      void disable() { _enabled = false; }

   private:
      // Data Members
      MemoryManager* _memory_manager;
      Cache* _L2_cache;
      CacheReplacementPolicy* _L2_cache_replacement_policy_obj;
      CacheHashFn* _L2_cache_hash_fn_obj;
      L1CacheCntlr* _L1_cache_cntlr;
      AddressHomeLookup* _dram_directory_home_lookup;

      // Outstanding ShmemReq info
      ShmemMsg _outstanding_shmem_msg;
      Time _outstanding_shmem_msg_time;

      // Is enabled?
      bool _enabled;

      // Eviction Counters
      // Measure clean/dirty evictions
      UInt64 _total_invalidations;
      UInt64 _total_evictions;
      UInt64 _total_dirty_evictions_exreq;
      UInt64 _total_clean_evictions_exreq;
      UInt64 _total_dirty_evictions_shreq;
      UInt64 _total_clean_evictions_shreq;

      // L2 cache operations
      void invalidateCacheLine(IntPtr address, PrL2CacheLineInfo& L2_cache_line_info);
      void readCacheLine(IntPtr address, Byte* data_buf);
      void insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component);

      // L1 cache operations
      void setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
      void invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address);
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

      void processBufferedShmemReqFromDramDirectory();

      // Utilities
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      ShmemPerfModel* getShmemPerfModel();

      // Dram Directory Home Lookup
      tile_id_t getHome(IntPtr address)
      { return _dram_directory_home_lookup->getHome(address); }

      // Performance Counters
      void initializeEvictionCounters();
      void initializeInvalidationCounters();
      void updateEvictionCounters(CacheState::Type inserted_cstate, CacheState::Type evicted_cstate);
      void updateInvalidationCounters();

      // Synchronization delay
      void addSynchronizationCost(MemComponent::Type mem_component);
   };

}
