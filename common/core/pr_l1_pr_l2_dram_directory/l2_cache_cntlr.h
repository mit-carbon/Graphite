#ifndef __L2_CACHE_CNTLR_H__
#define __L2_CACHE_CNTLR_H__

#include <map>

// Forward declarations
namespace PrL1PrL2DramDirectory
{
   class L1CacheCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "pr_l2_cache_block_info.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "lock.h"
#include "fixed_types.h"

namespace PrL1PrL2DramDirectory
{
   class L2CacheCntlr
   {
      private:
         // Data Members
         MemoryManager* m_memory_manager;
         Cache* m_l2_cache;
         L1CacheCntlr* m_l1_cache_cntlr;
         AddressHomeLookup* m_dram_directory_home_lookup;
         std::map<IntPtr, MemComponent::component_t> m_shmem_req_source_map;
         
         core_id_t m_core_id;
         UInt32 m_cache_block_size;

         Lock m_l2_cache_lock;
         Semaphore* m_mmu_sem;
         UInt32 m_locks_bitvec;

         PrL2CacheBlockInfo** m_stashed_block_info_list;
         IntPtr* m_stashed_address_list;
         UInt32 m_stash_size;

         // L2 Cache meta-data operations
         CacheState::cstate_t getCacheState(IntPtr address);
         void setCacheState(IntPtr address, CacheState::cstate_t cstate);

         // Whether it is present in the L1 cache
         void setCachedLoc(IntPtr address, MemComponent::component_t mem_component);
         void clearCachedLoc(IntPtr address, MemComponent::component_t mem_component);
         MemComponent::component_t getCachedLoc(IntPtr address);

         // L2 Cache data operations
         void invalidateCacheBlock(IntPtr address);
         void retrieveCacheBlock(IntPtr address, Byte* data_buf);
         void insertCacheBlock(IntPtr address, CacheState::cstate_t cstate, Byte* data_buf);

         // L1 Cache data manipulations
         void setCacheStateInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::cstate_t cstate);
         void invalidateCacheBlockInL1(MemComponent::component_t mem_component, IntPtr address);
         void insertCacheBlockInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::cstate_t cstate, Byte* data_buf);

         // Process Request from L1 Cache
         void processExReqFromL1Cache(ShmemMsg* shmem_msg);
         void processShReqFromL1Cache(ShmemMsg* shmem_msg);
         // Check if msg from L1 ends in the L2 cache
         bool shmemReqEndsInL2Cache(ShmemMsg::msg_t msg_type, IntPtr address);

         // Process Request from Dram Dir
         void processExRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processShRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processInvReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processFlushReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);
         void processWbReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg);

         // Some L2 Cache Block Info is always stashed for future use
         // This is done for making code cleaner as well as in
         // performance modelling
         PrL2CacheBlockInfo* getCacheBlockInfo(IntPtr address);
         SInt32 getStashIndex(IntPtr address);
         SInt32 getFreeStashIndex(IntPtr address);
         void dropStash(IntPtr address);
         void invalidateStash(void);

         // Acquiring and Releasing Locks
         void acquireNeededLocks(ShmemMsg::msg_t msg_type, IntPtr address);
         void releaseAllLocks(void);
         void setLockAcquired(MemComponent::component_t mem_component);
         void setLockReleased(MemComponent::component_t mem_component);
         bool isLockAcquired(MemComponent::component_t mem_component);
         bool areAllLocksReleased(void);

         // Cache Block Size
         UInt32 getCacheBlockSize() { return m_cache_block_size; }

         MemoryManager* getMemoryManager() { return m_memory_manager; }

         // Wake up User Thread
         void wakeUpUserThread(void);

         // Dram Directory Home Lookup
         core_id_t getHome(IntPtr address) { return m_dram_directory_home_lookup->getHome(address); }

      public:

         L2CacheCntlr(core_id_t core_id,
               MemoryManager* memory_manager,
               L1CacheCntlr* l1_cache_cntlr,
               Semaphore* mmu_sem,
               UInt32 cache_block_size,
               UInt32 l2_cache_size, UInt32 l2_cache_associativity,
               std::string l2_cache_replacement_policy,
               UInt32 l2_cache_access_time,
               bool l2_cache_track_detailed_counters,
               AddressHomeLookup* dram_directory_home_lookup);
         ~L2CacheCntlr();

         // Handle Request from L1 Cache - This is done for better simulator performance
         bool processShmemReqFromL1Cache(MemComponent::component_t req_mem_component, ShmemMsg::msg_t msg_type, IntPtr address);
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

#endif /* __L2_CACHE_CNTLR_H__ */
