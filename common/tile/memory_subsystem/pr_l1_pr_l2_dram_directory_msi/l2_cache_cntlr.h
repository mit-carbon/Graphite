#pragma once

#include <map>
#include <string>
using std::map;
using std::string;

// Forward declarations
namespace PrL1PrL2DramDirectoryMSI
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "pr_l2_cache_line_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "lock.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class L2CacheCntlr
   {
   public:
      L2CacheCntlr(MemoryManager* memory_manager,
                   L1CacheCntlr* l1_cache_cntlr,
                   AddressHomeLookup* dram_directory_home_lookup,
                   Semaphore* user_thread_sem,
                   Semaphore* network_thread_sem,
                   UInt32 cache_line_size,
                   UInt32 l2_cache_size,
                   UInt32 l2_cache_associativity,
                   string l2_cache_replacement_policy,
                   UInt32 l2_cache_access_delay,
                   bool l2_cache_track_miss_types,
                   volatile float frequency);
      ~L2CacheCntlr();

      Cache* getL2Cache() { return _l2_cache; }

      // Handle Request from L1 Cache - This is done for better simulator performance
      pair<bool,Cache::MissType> processShmemRequestFromL1Cache(MemComponent::Type mem_component, Core::mem_op_t mem_op_type, IntPtr address);
      // Write-through Cache. Hence needs to be written by the APP thread
      void writeCacheLine(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length);

      // Handle message from L1 Cache
      void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
      // Handle message from Dram Dir
      void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      // Acquiring and Releasing Locks
      void acquireLock();
      void releaseLock();
   
   private:
      // Data Members
      MemoryManager* _memory_manager;
      Cache* _l2_cache;
      L1CacheCntlr* _l1_cache_cntlr;
      AddressHomeLookup* _dram_directory_home_lookup;
      
      // Outstanding Miss information
      ShmemMsg _outstanding_shmem_msg;
      UInt64 _outstanding_shmem_msg_time;
      
      Lock _l2_cache_lock;
      Semaphore* _user_thread_sem;
      Semaphore* _network_thread_sem;

      // L2 cache operations
      void getCacheLineInfo(IntPtr address, PrL2CacheLineInfo* l2_cache_line_info);
      void setCacheLineInfo(IntPtr address, PrL2CacheLineInfo* l2_cache_line_info);
      void invalidateCacheLine(IntPtr address);
      void readCacheLine(IntPtr address, Byte* data_buf);
      void insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component);

      // L1 cache operations
      void setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate);
      void invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address);
      void insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf);

      // Insert cache line in hierarchy
      void insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, Byte* fill_buf);

      // Process Request from L1 Cache
      void processExReqFromL1Cache(ShmemMsg* shmem_msg);
      void processShReqFromL1Cache(ShmemMsg* shmem_msg);
      // Check if msg from L1 ends in the L2 cache
      pair<bool,Cache::MissType> operationPermissibleinL2Cache(Core::mem_op_t mem_op_type, IntPtr address, CacheState::Type cstate);

      // Process Request from Dram Dir
      void processExRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processShRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);
      void processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);

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

      MemComponent::Type acquireL1CacheLock(ShmemMsg::Type msg_type, IntPtr address);
   };

}
