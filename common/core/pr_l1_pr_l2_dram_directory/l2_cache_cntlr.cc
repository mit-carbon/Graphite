#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "log.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectory
{

L2CacheCntlr::L2CacheCntlr(core_id_t core_id,
      MemoryManager* memory_manager,
      L1CacheCntlr* l1_cache_cntlr,
      Semaphore* mmu_sem,
      UInt32 cache_block_size,
      UInt32 l2_cache_size, UInt32 l2_cache_associativity,
      std::string l2_cache_replacement_policy,
      UInt32 l2_cache_access_time,
      bool l2_cache_track_detailed_counters,
      AddressHomeLookup* dram_directory_home_lookup):
   m_memory_manager(memory_manager),
   m_l1_cache_cntlr(l1_cache_cntlr),
   m_dram_directory_home_lookup(dram_directory_home_lookup),
   m_core_id(core_id),
   m_cache_block_size(cache_block_size),
   m_mmu_sem(mmu_sem),
   m_locks_bitvec(0),
   m_stash_size(2)
{
   m_l2_cache = new Cache("l2_cache",
         l2_cache_size, 
         l2_cache_associativity, cache_block_size, 
         l2_cache_replacement_policy, 
         CacheBase::PR_L2_CACHE,
         l2_cache_access_time,
         l2_cache_track_detailed_counters,
         NULL /* Change this later */); 

   // Initialized the Stashed info
   m_stashed_block_info_list = new PrL2CacheBlockInfo*[m_stash_size];
   m_stashed_address_list = new IntPtr[m_stash_size];
   for (SInt32 i = 0; i < (SInt32) m_stash_size; i++)
   {
      m_stashed_block_info_list[i] = (PrL2CacheBlockInfo*) NULL;
      m_stashed_address_list[i] = INVALID_ADDRESS;
   }
}

L2CacheCntlr::~L2CacheCntlr()
{
   delete m_l2_cache;

   delete [] m_stashed_block_info_list;
   delete [] m_stashed_address_list;
}

CacheState::cstate_t
L2CacheCntlr::getCacheState(IntPtr address)
{
   PrL2CacheBlockInfo* l2_cache_block_info = getCacheBlockInfo(address);
   if (l2_cache_block_info == NULL)
      return CacheState::INVALID;
   else
      return l2_cache_block_info->getCState();
}

void
L2CacheCntlr::setCacheState(IntPtr address, CacheState::cstate_t cstate)
{
   PrL2CacheBlockInfo* l2_cache_block_info = getCacheBlockInfo(address);
   assert(l2_cache_block_info);
   l2_cache_block_info->setCState(cstate);
}

void
L2CacheCntlr::setCachedLoc(IntPtr address, MemComponent::component_t mem_component)
{
   PrL2CacheBlockInfo* l2_cache_block_info = getCacheBlockInfo(address);
   assert(l2_cache_block_info);

   LOG_ASSERT_ERROR(l2_cache_block_info->getCachedLoc() == MemComponent::INVALID_MEM_COMPONENT, "l2_cache_block_info->cached_loc(%u)", l2_cache_block_info->getCachedLoc());
   
   l2_cache_block_info->setCachedLoc(mem_component);
}

void
L2CacheCntlr::clearCachedLoc(IntPtr address, MemComponent::component_t mem_component)
{
   PrL2CacheBlockInfo* l2_cache_block_info = getCacheBlockInfo(address);
   assert(l2_cache_block_info);
   assert(l2_cache_block_info->getCachedLoc() == mem_component);
   l2_cache_block_info->setCachedLoc(MemComponent::INVALID_MEM_COMPONENT);
}

MemComponent::component_t
L2CacheCntlr::getCachedLoc(IntPtr address)
{
   PrL2CacheBlockInfo* l2_cache_block_info = getCacheBlockInfo(address);
   if (l2_cache_block_info == NULL)
      return MemComponent::INVALID_MEM_COMPONENT;
   else
      return l2_cache_block_info->getCachedLoc();
}

void
L2CacheCntlr::invalidateCacheBlock(IntPtr address)
{
   m_l2_cache->invalidateSingleLine(address);
}

void
L2CacheCntlr::retrieveCacheBlock(IntPtr address, Byte* data_buf)
{
   __attribute(__unused__) PrL2CacheBlockInfo* l2_cache_block_info = (PrL2CacheBlockInfo*) m_l2_cache->accessSingleLine(address, Cache::LOAD, data_buf, getCacheBlockSize());
   assert(l2_cache_block_info);
}

void
L2CacheCntlr::writeCacheBlock(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length)
{
   __attribute(__unused__) PrL2CacheBlockInfo* l2_cache_block_info = (PrL2CacheBlockInfo*) m_l2_cache->accessSingleLine(address + offset, Cache::STORE, data_buf, data_length);
   assert(l2_cache_block_info);
}

void
L2CacheCntlr::insertCacheBlock(IntPtr address, CacheState::cstate_t cstate, Byte* data_buf)
{
   // TODO: Validate assumption that L1 caches are write-through
   bool eviction;
   IntPtr evict_address;
   PrL2CacheBlockInfo evict_block_info;
   Byte evict_buf[getCacheBlockSize()];

   m_l2_cache->insertSingleLine(address, data_buf,
         &eviction, &evict_address, &evict_block_info, evict_buf);
   setCacheState(address, cstate);


   if (eviction)
   {
      LOG_PRINT("Eviction: addr(0x%x)", evict_address);
      invalidateCacheBlockInL1(evict_block_info.getCachedLoc(), evict_address); 

      UInt32 home_node_id = getHome(evict_address);
      if (evict_block_info.isDirty())
      {
         // Send back the data also
         assert(evict_block_info.getCState() == CacheState::MODIFIED);
         getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, home_node_id, evict_address, evict_buf, getCacheBlockSize());
      }
      else
      {
         assert(evict_block_info.getCState() == CacheState::SHARED);
         getMemoryManager()->sendMsg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, home_node_id, evict_address);
      }
   }
}

void
L2CacheCntlr::setCacheStateInL1(MemComponent::component_t mem_component, IntPtr address, CacheState::cstate_t cstate)
{
   if (mem_component != MemComponent::INVALID_MEM_COMPONENT)
      m_l1_cache_cntlr->setCacheState(mem_component, address, cstate);
}

void
L2CacheCntlr::invalidateCacheBlockInL1(MemComponent::component_t mem_component, IntPtr address)
{
   if (mem_component != MemComponent::INVALID_MEM_COMPONENT)
      m_l1_cache_cntlr->invalidateCacheBlock(mem_component, address);
}

void
L2CacheCntlr::insertCacheBlockInL1(MemComponent::component_t mem_component,
      IntPtr address, CacheState::cstate_t cstate, Byte* data_buf)
{
   bool eviction;
   IntPtr evict_address;

   // Insert the Cache Block in L1 Cache
   m_l1_cache_cntlr->insertCacheBlock(mem_component, address, cstate, data_buf, &eviction, &evict_address);

   // Set the Present bit in L2 Cache corresponding to the inserted block
   LOG_PRINT("insertCacheBlockInL1(): Starting to set cached loc for new cache block");
   setCachedLoc(address, mem_component);
   LOG_PRINT("insertCacheBlockInL1(): Finished setting cached loc for new cache block");

   if (eviction)
   {
      // Clear the Present bit in L2 Cache corresponding to the evicted block
      clearCachedLoc(evict_address, mem_component);
   }
}

bool
L2CacheCntlr::processShmemReqFromL1Cache(MemComponent::component_t req_mem_component, ShmemMsg::msg_t msg_type, IntPtr address)
{
   bool shmem_req_ends_in_l2_cache = shmemReqEndsInL2Cache(msg_type, address);
   if (shmem_req_ends_in_l2_cache)
   {
      CacheState::cstate_t cstate;
      Byte data_buf[getCacheBlockSize()];
      
      cstate = getCacheState(address);
      retrieveCacheBlock(address, data_buf);

      insertCacheBlockInL1(req_mem_component, address, cstate, data_buf);
   }
   
   // Remove Stashed contents
   invalidateStash();

   return shmem_req_ends_in_l2_cache;
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;
   ShmemMsg::msg_t shmem_msg_type = shmem_msg->m_msg_type;
   MemComponent::component_t sender_mem_component = shmem_msg->m_sender_mem_component;

   acquireNeededLocks(shmem_msg_type, address);
  
   assert(shmem_msg->m_data_buf == NULL);
   assert(shmem_msg->m_data_length == 0);

   m_shmem_req_source_map[address] = sender_mem_component;
   switch (shmem_msg_type)
   {
      case ShmemMsg::EX_REQ:
         processExReqFromL1Cache(shmem_msg);
         break;

      case ShmemMsg::SH_REQ:
         processShReqFromL1Cache(shmem_msg);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized shmem msg type (%u)", shmem_msg_type);
         break;
   }

   releaseAllLocks();
}

void
L2CacheCntlr::processExReqFromL1Cache(ShmemMsg* shmem_msg)
{
   // We need to send a request to the Dram Directory Cache
   IntPtr address = shmem_msg->m_address;

   CacheState::cstate_t cstate = getCacheState(address);
   assert((cstate == CacheState::INVALID) || (cstate == CacheState::SHARED));
   if (cstate == CacheState::SHARED)
   {
      // This will clear the 'Present' bit also
      invalidateCacheBlock(address);
      getMemoryManager()->sendMsg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, getHome(address), address);
   }

   getMemoryManager()->sendMsg(ShmemMsg::EX_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, getHome(address), address);
}

void
L2CacheCntlr::processShReqFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;

   getMemoryManager()->sendMsg(ShmemMsg::SH_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, getHome(address), address);
}

void
L2CacheCntlr::handleMsgFromDramDirectory(
      core_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::msg_t shmem_msg_type = shmem_msg->m_msg_type;

   // Acquire Locks
   acquireNeededLocks(shmem_msg_type, shmem_msg->m_address);

   switch (shmem_msg_type)
   {
      case ShmemMsg::EX_REP:
         processExRepFromDramDirectory(sender, shmem_msg);
         break;
      case ShmemMsg::SH_REP:
         processShRepFromDramDirectory(sender, shmem_msg);
         break;
      case ShmemMsg::INV_REQ:
         processInvReqFromDramDirectory(sender, shmem_msg);
         break;
      case ShmemMsg::FLUSH_REQ:
         processFlushReqFromDramDirectory(sender, shmem_msg);
         break;
      case ShmemMsg::WB_REQ:
         processWbReqFromDramDirectory(sender, shmem_msg);
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized msg type: %u", shmem_msg_type);
         break;
   }
   // Invalidate the Stashed Meta-data pointers
   LOG_PRINT("handleMsgFromDramDirectory: Starting to invalidate Stash");
   invalidateStash();
   LOG_PRINT("handleMsgFromDramDirectory: Finished invalidating Stash");

   // Release Locks
   releaseAllLocks();
}

void
L2CacheCntlr::processExRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;
   Byte* data_buf = shmem_msg->m_data_buf;

   // Insert Cache Block in L2 Cache
   LOG_PRINT("processExRep: starting to insert Cache Block in L2 Cache");
   insertCacheBlock(address, CacheState::MODIFIED, data_buf);
   LOG_PRINT("processExRep: finished inserting Cache Block in L2 Cache");

   LOG_ASSERT_ERROR(getCachedLoc(address) == MemComponent::INVALID_MEM_COMPONENT,
      "processExRep: getCachedLoc(0x%x) = %u", address, getCachedLoc(address));

   // Insert Cache Block in L1 Cache
   // Support for non-blocking caches can be added in this way
   MemComponent::component_t mem_component = m_shmem_req_source_map[address];
   assert (mem_component == MemComponent::L1_DCACHE);
   
   LOG_PRINT("processExRep: starting to insert Cache Block in L1 Cache");
   insertCacheBlockInL1(mem_component, address, CacheState::MODIFIED, data_buf);
   LOG_PRINT("processExRep: finished inserting Cache Block in L1 Cache");

   wakeUpUserThread();
}

void
L2CacheCntlr::processShRepFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;
   Byte* data_buf = shmem_msg->m_data_buf;

   // Insert Cache Block in L2 Cache
   insertCacheBlock(address, CacheState::SHARED, data_buf);

   // Insert Cache Block in L1 Cache
   // Support for non-blocking caches can be added in this way
   MemComponent::component_t mem_component = m_shmem_req_source_map[address];
   insertCacheBlockInL1(mem_component, address, CacheState::SHARED, data_buf);
   
   wakeUpUserThread();
}

void
L2CacheCntlr::processInvReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;

   CacheState::cstate_t cstate = getCacheState(address);
   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::SHARED);
  
      // Invalidate the line in L1 Cache
      invalidateCacheBlockInL1(getCachedLoc(address), address);
      // Invalidate the line in the L2 cache also
      invalidateCacheBlock(address);

      getMemoryManager()->sendMsg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, sender, address);
   }
}

void
L2CacheCntlr::processFlushReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;

   CacheState::cstate_t cstate = getCacheState(address);
   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::MODIFIED);
      
      // Invalidate the line in L1 Cache
      invalidateCacheBlockInL1(getCachedLoc(address), address);

      // Flush the line
      Byte data_buf[getCacheBlockSize()];
      retrieveCacheBlock(address, data_buf);
      invalidateCacheBlock(address);

      getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, sender, address, data_buf, getCacheBlockSize());
   }
}

void
L2CacheCntlr::processWbReqFromDramDirectory(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->m_address;

   CacheState::cstate_t cstate = getCacheState(address);
   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::MODIFIED);
 
      // Set the Appropriate Cache State in L1 also
      setCacheStateInL1(getCachedLoc(address), address, CacheState::SHARED);

      // Write-Back the line
      Byte data_buf[getCacheBlockSize()];
      retrieveCacheBlock(address, data_buf);
      setCacheState(address, CacheState::SHARED);

      getMemoryManager()->sendMsg(ShmemMsg::WB_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIR, sender, address, data_buf, getCacheBlockSize());
   }
}

bool
L2CacheCntlr::shmemReqEndsInL2Cache(ShmemMsg::msg_t shmem_msg_type, IntPtr address)
{
   CacheState::cstate_t cstate = getCacheState(address);
   switch (shmem_msg_type)
   {
      case ShmemMsg::EX_REQ:
         return CacheState(cstate).writable();

      case ShmemMsg::SH_REQ:
         return CacheState(cstate).readable();

      default:
         LOG_PRINT_ERROR("Unsupported Shmem Msg Type: %u", shmem_msg_type);
         return false;
   }
}

PrL2CacheBlockInfo* 
L2CacheCntlr::getCacheBlockInfo(IntPtr address)
{
   // Some addresses are always cached
   SInt32 stash_index = getStashIndex(address);
   if (stash_index != -1)
   {
      return m_stashed_block_info_list[stash_index];
   }
   else
   {
      SInt32 free_stash_index = getFreeStashIndex(address);
      assert(free_stash_index != -1);
      m_stashed_block_info_list[free_stash_index] = (PrL2CacheBlockInfo*) m_l2_cache->peekSingleLine(address);
      return m_stashed_block_info_list[free_stash_index];
   }
}

SInt32
L2CacheCntlr::getStashIndex(IntPtr address)
{
   for (SInt32 i = 0; i < (SInt32) m_stash_size; i++)
   {
      if (m_stashed_address_list[i] == address)
         return i;
   }
   return -1;
}

SInt32
L2CacheCntlr::getFreeStashIndex(IntPtr address)
{
   for (SInt32 i = 0; i < (SInt32) m_stash_size; i++)
   {
      if (m_stashed_address_list[i] == INVALID_ADDRESS)
         return i;
   }
   return -1;
}

void
L2CacheCntlr::dropStash(IntPtr address)
{
   for (SInt32 i = 0; i < (SInt32) m_stash_size; i++)
   {
      if (m_stashed_address_list[i] == address)
      {
         m_stashed_address_list[i] = INVALID_ADDRESS;
         return;
      }
   }
   // Should not reach here
   assert(false);
}

void
L2CacheCntlr::invalidateStash()
{
   for (SInt32 i = 0; i < (SInt32) m_stash_size; i++)
      m_stashed_address_list[i] = INVALID_ADDRESS;
}

void
L2CacheCntlr::acquireNeededLocks(ShmemMsg::msg_t msg_type, IntPtr address)
{
   LOG_PRINT("Entering acquireNeededLocks()");

   assert(areAllLocksReleased());

   switch(msg_type)
   {
      case ShmemMsg::EX_REQ:
         // From L1 Cache - We unnecessarily double the access latency here
         acquireLock();
         setLockAcquired(MemComponent::L2_CACHE);
         break;

      case ShmemMsg::SH_REQ:
         // No locks needed
         break;

      case ShmemMsg::EX_REP:
      case ShmemMsg::SH_REP:

         {
            MemComponent::component_t mem_component = m_shmem_req_source_map[address];

            m_l1_cache_cntlr->acquireLock(mem_component);
            setLockAcquired(mem_component);
            
            acquireLock();
            setLockAcquired(MemComponent::L2_CACHE);
         }
         break;

      case ShmemMsg::INV_REQ:
      case ShmemMsg::FLUSH_REQ:
      case ShmemMsg::WB_REQ:

         {
            acquireLock();
            setLockAcquired(MemComponent::L2_CACHE);

            MemComponent::component_t mem_component = getCachedLoc(address);
            if (mem_component != MemComponent::INVALID_MEM_COMPONENT)
            {
               releaseLock();
               setLockReleased(MemComponent::L2_CACHE);

               m_l1_cache_cntlr->acquireLock(mem_component);
               setLockAcquired(mem_component);

               acquireLock();
               setLockAcquired(MemComponent::L2_CACHE);

               /*
               // Now, the L1 cache data may have been invalidated in the meantime
               // So, re-check and release locks if need be 
               MemComponent::component_t new_mem_component = getCachedLoc(address);
               if (new_mem_component != mem_component)
               {
                  assert(new_mem_component == MemComponent::INVALID_MEM_COMPONENT);
                  m_l1_cache_cntlr->releaseLock(mem_component);
                  setLockReleased(mem_component);
               }
               */
            }
         }
         
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized shmem msg type (%u)", msg_type);
         break;
   }
   
   LOG_PRINT("Leaving acquireNeededLocks()");
}

void
L2CacheCntlr::releaseAllLocks()
{
   if (isLockAcquired(MemComponent::L2_CACHE))
   {
      releaseLock();
      setLockReleased(MemComponent::L2_CACHE);
   }
   if (isLockAcquired(MemComponent::L1_DCACHE))
   {
      m_l1_cache_cntlr->releaseLock(MemComponent::L1_DCACHE);
      setLockReleased(MemComponent::L1_DCACHE);
   }
   if (isLockAcquired(MemComponent::L1_ICACHE))
   {
      m_l1_cache_cntlr->releaseLock(MemComponent::L1_ICACHE);
      setLockReleased(MemComponent::L1_ICACHE);
   }
   assert(areAllLocksReleased());
}

void
L2CacheCntlr::setLockAcquired(MemComponent::component_t mem_component)
{
   m_locks_bitvec |= ((UInt32) mem_component);
}

void
L2CacheCntlr::setLockReleased(MemComponent::component_t mem_component)
{
   m_locks_bitvec &= ~((UInt32) mem_component);
}

bool
L2CacheCntlr::isLockAcquired(MemComponent::component_t mem_component)
{
   return (bool) (m_locks_bitvec & ((UInt32) mem_component));
}

bool
L2CacheCntlr::areAllLocksReleased()
{
   return (m_locks_bitvec == 0);
}

void
L2CacheCntlr::acquireLock()
{
   m_l2_cache_lock.acquire();
}

void
L2CacheCntlr::releaseLock()
{
   m_l2_cache_lock.release();
}

void
L2CacheCntlr::wakeUpUserThread()
{
   m_mmu_sem->signal();
}

}
