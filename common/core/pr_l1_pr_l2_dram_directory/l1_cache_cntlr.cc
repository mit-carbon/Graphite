#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h" 
#include "memory_manager.h"

namespace PrL1PrL2DramDirectory
{

L1CacheCntlr::L1CacheCntlr(core_id_t core_id,
      MemoryManager* memory_manager,
      Semaphore* mmu_sem,
      UInt32 cache_block_size,
      UInt32 l1_icache_size, UInt32 l1_icache_associativity,
      std::string l1_icache_replacement_policy,
      UInt32 l1_icache_data_access_time,
      UInt32 l1_icache_tags_access_time,
      std::string l1_icache_perf_model_type,
      UInt32 l1_dcache_size, UInt32 l1_dcache_associativity,
      std::string l1_dcache_replacement_policy,
      bool l1_dcache_track_detailed_cache_counters,
      UInt32 l1_dcache_data_access_time,
      UInt32 l1_dcache_tags_access_time,
      std::string l1_dcache_perf_model_type,
      ShmemPerfModel* shmem_perf_model) :
   m_memory_manager(memory_manager),
   m_l2_cache_cntlr(NULL),
   m_core_id(core_id),
   m_cache_block_size(cache_block_size),
   m_mmu_sem(mmu_sem),
   m_shmem_perf_model(shmem_perf_model)
{
   m_l1_icache = new Cache("l1_icache",
         l1_icache_size,
         l1_icache_associativity, 
         m_cache_block_size,
         l1_icache_replacement_policy,
         CacheBase::PR_L1_CACHE,
         false,
         l1_icache_data_access_time,
         l1_icache_tags_access_time,
         l1_icache_perf_model_type,
         m_shmem_perf_model);
   m_l1_dcache = new Cache("l1_dcache",
         l1_dcache_size,
         l1_dcache_associativity, 
         m_cache_block_size,
         l1_dcache_replacement_policy,
         CacheBase::PR_L1_CACHE,
         l1_dcache_track_detailed_cache_counters,
         l1_dcache_data_access_time,
         l1_dcache_tags_access_time,
         l1_dcache_perf_model_type,
         m_shmem_perf_model);
}

L1CacheCntlr::~L1CacheCntlr()
{
   delete m_l1_icache;
   delete m_l1_dcache;
}      

void
L1CacheCntlr::setL2CacheCntlr(L2CacheCntlr* l2_cache_cntlr)
{
   m_l2_cache_cntlr = l2_cache_cntlr;
}

bool
L1CacheCntlr::processMemOpFromCore(
      MemComponent::component_t mem_component,
      Core::lock_signal_t lock_signal,
      Core::mem_op_t mem_op_type, 
      IntPtr ca_address, UInt32 offset,
      Byte* data_buf, UInt32 data_length)
{
   LOG_PRINT("processMemOpFromCore(), lock_signal(%u), mem_op_type(%u), ca_address(0x%x)",
         lock_signal, mem_op_type, ca_address);

   bool l1_cache_hit = true;
   while(1)
   {
      if (lock_signal != Core::UNLOCK)
         acquireLock(mem_component);

      if (operationPermissibleinL1Cache(mem_component, ca_address, mem_op_type))
      {
         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);
                 
         if (lock_signal != Core::LOCK)
            releaseLock(mem_component);
         return l1_cache_hit;
      }

      if (lock_signal == Core::UNLOCK)
         LOG_PRINT_ERROR("Expected to find address(0x%x) in L1 Cache", ca_address);

      // Invalidate the cache block before passing the request to L2 Cache
      invalidateCacheBlock(mem_component, ca_address);

      m_l2_cache_cntlr->acquireLock();
 
      ShmemMsg::msg_t shmem_msg_type = getShmemMsgType(mem_op_type);

      if (m_l2_cache_cntlr->processShmemReqFromL1Cache(mem_component, shmem_msg_type, ca_address))
      {
         m_l2_cache_cntlr->releaseLock();
         
         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);

         if (lock_signal != Core::LOCK)
            releaseLock(mem_component);
         return false;
      }

      l1_cache_hit = false;
      
      m_l2_cache_cntlr->releaseLock();
      releaseLock(mem_component);
      
      // Send out a request to the network thread for the cache data
      getMemoryManager()->sendMsg(shmem_msg_type, 
            mem_component, MemComponent::L2_CACHE,
            m_core_id, ca_address);
 
      waitForNetworkThread();
   }

   LOG_PRINT_ERROR("Should not reach here");
   return false;
}

void
L1CacheCntlr::accessCache(MemComponent::component_t mem_component,
      Core::mem_op_t mem_op_type, IntPtr ca_address, UInt32 offset,
      Byte* data_buf, UInt32 data_length)
{
   Cache* l1_cache = getL1Cache(mem_component);
   switch (mem_op_type)
   {
      case Core::READ:
      case Core::READ_EX:
         l1_cache->accessSingleLine(ca_address + offset, Cache::LOAD, data_buf, data_length);
         break;

      case Core::WRITE:
         l1_cache->accessSingleLine(ca_address + offset, Cache::STORE, data_buf, data_length);
         // Write-through cache - Write the L2 Cache also
         m_l2_cache_cntlr->acquireLock();
         m_l2_cache_cntlr->writeCacheBlock(ca_address, offset, data_buf, data_length);
         m_l2_cache_cntlr->releaseLock();
         break;

      default:
         LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
         break;
   }
}

bool
L1CacheCntlr::operationPermissibleinL1Cache(
      MemComponent::component_t mem_component, 
      IntPtr address, Core::mem_op_t mem_op_type)
{
   // TODO: Verify why this works
   CacheState::cstate_t cstate = getCacheState(mem_component, address);
   switch (mem_op_type)
   {
      case Core::READ:
         return CacheState(cstate).readable();

      case Core::READ_EX:
      case Core::WRITE:
         return CacheState(cstate).writable();

      default:
         LOG_PRINT_ERROR("Unsupported mem_op_type: %u", mem_op_type);
         return false;
   }
}

void
L1CacheCntlr::insertCacheBlock(MemComponent::component_t mem_component,
      IntPtr address, CacheState::cstate_t cstate, Byte* data_buf,
      bool* eviction_ptr, IntPtr* evict_address_ptr)
{
   __attribute(__unused__) PrL1CacheBlockInfo evict_block_info;
   __attribute(__unused__) Byte evict_buf[getCacheBlockSize()];

   Cache* l1_cache = getL1Cache(mem_component);
   l1_cache->insertSingleLine(address, data_buf,
         eviction_ptr, evict_address_ptr, &evict_block_info, evict_buf);
   setCacheState(mem_component, address, cstate);
}

CacheState::cstate_t
L1CacheCntlr::getCacheState(MemComponent::component_t mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);

   PrL1CacheBlockInfo* l1_cache_block_info = (PrL1CacheBlockInfo*) l1_cache->peekSingleLine(address);
   return (l1_cache_block_info == NULL) ? CacheState::INVALID : l1_cache_block_info->getCState(); 
}

void
L1CacheCntlr::setCacheState(MemComponent::component_t mem_component, IntPtr address, CacheState::cstate_t cstate)
{
   Cache* l1_cache = getL1Cache(mem_component);

   PrL1CacheBlockInfo* l1_cache_block_info = (PrL1CacheBlockInfo*) l1_cache->peekSingleLine(address);
   assert(l1_cache_block_info);

   l1_cache_block_info->setCState(cstate);
}

void
L1CacheCntlr::invalidateCacheBlock(MemComponent::component_t mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);

   l1_cache->invalidateSingleLine(address);
}

ShmemMsg::msg_t
L1CacheCntlr::getShmemMsgType(Core::mem_op_t mem_op_type)
{
   switch(mem_op_type)
   {
      case Core::READ:
         return ShmemMsg::SH_REQ;

      case Core::READ_EX:
      case Core::WRITE:
         return ShmemMsg::EX_REQ;

      default:
         LOG_PRINT_ERROR("Unsupported Mem Op Type(%u)", mem_op_type);
         return ShmemMsg::INVALID_MSG_TYPE;
   }
}

Cache*
L1CacheCntlr::getL1Cache(MemComponent::component_t mem_component)
{
   switch(mem_component)
   {
      case MemComponent::L1_ICACHE:
         return m_l1_icache;

      case MemComponent::L1_DCACHE:
         return m_l1_dcache;

      default:
         LOG_PRINT_ERROR("Unrecognized Memory Component(%u)", mem_component);
         return NULL;
   }
}

void
L1CacheCntlr::acquireLock(MemComponent::component_t mem_component)
{
   switch(mem_component)
   {
      case MemComponent::L1_ICACHE:
         m_l1_icache_lock.acquire();
         break;
      case MemComponent::L1_DCACHE:
         m_l1_dcache_lock.acquire();
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
         break;
   }

}

void
L1CacheCntlr::releaseLock(MemComponent::component_t mem_component)
{
   switch(mem_component)
   {
      case MemComponent::L1_ICACHE:
         m_l1_icache_lock.release();
         break;
      case MemComponent::L1_DCACHE:
         m_l1_dcache_lock.release();
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
         break;
   }
}

void
L1CacheCntlr::acquireAllLocks()
{
   m_l1_icache_lock.acquire();
   m_l1_dcache_lock.acquire();
}

void
L1CacheCntlr::releaseAllLocks()
{
   m_l1_icache_lock.release();
   m_l1_dcache_lock.release();
}

void
L1CacheCntlr::waitForNetworkThread()
{
   m_mmu_sem->wait();
}

}
