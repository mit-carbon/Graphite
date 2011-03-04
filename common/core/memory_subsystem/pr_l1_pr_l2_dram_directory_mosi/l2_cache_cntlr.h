#pragma once

// Forward declarations
namespace PrL1PrL2DramDirectoryMOSI
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "pr_l2_cache_block_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "shmem_req.h"
#include "mem_component.h"
#include "semaphore.h"
#include "lock.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class L2CacheCntlr
   {
      private:
         // Data Members
         MemoryManager* m_memory_manager;
         Cache* m_l2_cache;
         L1CacheCntlr* m_l1_cache_cntlr;
         AddressHomeLookup* m_dram_directory_home_lookup;

         // Outstanding ShmemReq info
         ShmemMsg m_outstanding_shmem_msg;

         core_id_t m_core_id;
         UInt32 m_cache_block_size;

         Lock m_l2_cache_lock;
         Semaphore* m_user_thread_sem;
         Semaphore* m_network_thread_sem;

         ShmemPerfModel* m_shmem_perf_model;

         // Buffered Msg Req From Dram Directory
         ShmemReq m_buffered_shmem_req;

         // L2 Cache meta-data operations
         CacheState::cstate_t getCacheState(PrL2CacheBlockInfo* l2_cache_block_info);
         void setCacheState(PrL2CacheBlockInfo* l2_cache_block_info, CacheState::cstate_t cstate);

         PrL2CacheBlockInfo* getCacheBlockInfo(IntPtr address);
         
         // L2 Cache data operations
         void invalidateCacheBlock(IntPtr address);
         void retrieveCacheBlock(IntPtr address, Byte* data_buf);
         PrL2CacheBlockInfo* insertCacheBlock(IntPtr address, CacheState::cstate_t cstate, Byte* data_buf);

         // L1 Cache data manipulations
         CacheState::cstate_t getCacheStateInL1(MemComponent::component_t mem_component, IntPtr address);
         void setCacheStateInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::cstate_t cstate);
         void invalidateCacheBlockInL1(MemComponent::component_t mem_component, IntPtr address);
         void insertCacheBlockInL1(MemComponent::component_t mem_component, IntPtr address, PrL2CacheBlockInfo* l2_cache_block_info, CacheState::cstate_t cstate, Byte* data_buf);

         // Process Request from L1 Cache
         // Check if msg from L1 ends in the L2 cache
         bool shmemReqEndsInL2Cache(ShmemMsg::msg_t msg_type, CacheState::cstate_t cstate, bool modeled);

         // Process Request from Dram Dir
         void processExRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processShRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processUpgradeRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processInvReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processFlushReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processWbReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processInvFlushCombinedReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);

         void processBufferedShmemReqFromDramDirectory(void);

         // Cache Block Size
         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         MemoryManager* getMemoryManager() { return m_memory_manager; }
         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

         // Wake up User Thread
         void wakeUpUserThread(void);
         // Wait for User Thread
         void waitForUserThread(void);

         // Dram Directory Home Lookup
         core_id_t getHome(IntPtr address) { return m_dram_directory_home_lookup->getHome(address); }

         MemComponent::component_t acquireL1CacheLock(ShmemMsg::msg_t msg_type, IntPtr address);

      public:

         L2CacheCntlr(core_id_t core_id,
               MemoryManager* memory_manager,
               L1CacheCntlr* l1_cache_cntlr,
               AddressHomeLookup* dram_directory_home_lookup,
               Semaphore* user_thread_sem,
               Semaphore* network_thread_sem,
               UInt32 cache_block_size,
               UInt32 l2_cache_size, UInt32 l2_cache_associativity,
               std::string l2_cache_replacement_policy,
               UInt32 l2_cache_access_delay,
               volatile float frequency,
               ShmemPerfModel* shmem_perf_model);
         
         ~L2CacheCntlr();

         Cache* getL2Cache() { return m_l2_cache; }

         // Handle Request from L1 Cache - This is done for better simulator performance
         bool processShmemReqFromL1Cache(MemComponent::component_t req_mem_component, ShmemMsg::msg_t msg_type, IntPtr address, bool modeled);
         // Write-through Cache. Hence needs to be written by user thread
         void writeCacheBlock(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length);

         // Handle message from L1 Cache
         void handleMsgFromL1Cache(ShmemMsg* shmem_msg);
         // Handle message from Dram Dir
         void handleMsgFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         // Acquiring and Releasing Locks
         void acquireLock(void);
         void releaseLock(void);
   };

}
