#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h" 
#include "memory_manager.h"
#include "cache_level.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

L1CacheCntlr::L1CacheCntlr(MemoryManager* memory_manager,
                           UInt32 cache_line_size,
                           UInt32 l1_icache_size,
                           UInt32 l1_icache_associativity,
                           string l1_icache_replacement_policy,
                           UInt32 l1_icache_access_delay,
                           bool l1_icache_track_miss_types,
                           UInt32 l1_dcache_size,
                           UInt32 l1_dcache_associativity,
                           string l1_dcache_replacement_policy,
                           UInt32 l1_dcache_access_delay,
                           bool l1_dcache_track_miss_types,
                           float frequency)
   : _memory_manager(memory_manager)
   , _l2_cache_cntlr(NULL)
{
   _l1_icache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l1_icache_replacement_policy, l1_icache_size, l1_icache_associativity, cache_line_size);
   _l1_dcache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l1_dcache_replacement_policy, l1_dcache_size, l1_dcache_associativity, cache_line_size);
   _l1_icache_hash_fn_obj = new CacheHashFn(l1_icache_size, l1_icache_associativity, cache_line_size);
   _l1_dcache_hash_fn_obj = new CacheHashFn(l1_dcache_size, l1_dcache_associativity, cache_line_size);
   
   _l1_icache = new Cache("L1-I",
         HYBRID_PROTOCOL__PP_MOSI__SS,
         Cache::INSTRUCTION_CACHE,
         L1,
         Cache::UNDEFINED_WRITE_POLICY,
         l1_icache_size,
         l1_icache_associativity, 
         cache_line_size,
         _l1_icache_replacement_policy_obj,
         _l1_icache_hash_fn_obj,
         l1_icache_access_delay,
         frequency,
         l1_icache_track_miss_types);
   _l1_dcache = new Cache("L1-D",
         HYBRID_PROTOCOL__PP_MOSI__SS,
         Cache::DATA_CACHE,
         L2,
         Cache::WRITE_THROUGH,
         l1_dcache_size,
         l1_dcache_associativity, 
         cache_line_size,
         _l1_dcache_replacement_policy_obj,
         _l1_dcache_hash_fn_obj,
         l1_dcache_access_delay,
         frequency,
         l1_dcache_track_miss_types);
}

L1CacheCntlr::~L1CacheCntlr()
{
   delete _l1_icache;
   delete _l1_dcache;
   delete _l1_icache_replacement_policy_obj;
   delete _l1_dcache_replacement_policy_obj;
   delete _l1_icache_hash_fn_obj;
   delete _l1_dcache_hash_fn_obj;
}      

void
L1CacheCntlr::setL2CacheCntlr(L2CacheCntlr* l2_cache_cntlr)
{
   _l2_cache_cntlr = l2_cache_cntlr;
}

bool
L1CacheCntlr::processMemOpFromCore(MemComponent::Type mem_component,
                                   Core::lock_signal_t lock_signal,
                                   Core::mem_op_t mem_op_type, 
                                   IntPtr ca_address, UInt32 offset,
                                   Byte* data_buf, UInt32 data_length,
                                   bool modeled)
{
   // It would be great if we had a way to predict the cache coherence protocol by
   // looking at the instruction address (EIP). This would imply that we need a way to roll-back
   // our actions if the prediction was wrong.
   LOG_PRINT("processMemOpFromCore(), lock_signal(%u), mem_op_type(%u), ca_address(%#lx), offset(%u), data_length(%u)",
             lock_signal, mem_op_type, ca_address, offset, data_length);

   // Acquire the lock for the L1-I/L1-D cache
   acquireLock(mem_component);

   pair<bool,Cache::MissType> l1_cache_status = getMemOpStatusInL1Cache(mem_component, ca_address, mem_op_type);
   bool l1_cache_hit = !l1_cache_status.first;
   if (l1_cache_hit)
   {
      accessCache(mem_component, lock_signal, mem_op_type, ca_address, offset, data_buf, data_length);
      return true;
   }

   _l2_cache_cntlr->processMemOpFromL1Cache(mem_component, lock_signal, mem_op_type, 
                                            ca_address, offset, data_buf, data_length,
                                            modeled);
   return false;
}

pair<bool,Cache::MissType>
L1CacheCntlr::getMemOpStatusInL1Cache(MemComponent::Type mem_component, IntPtr address, Core::mem_op_t mem_op_type)
{
   Cache* l1_cache = getL1Cache(mem_component);
   HybridL1CacheLineInfo l1_cache_line_info;

   while (1)
   {
      l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
      bool locked = l1_cache_line_info.isLocked();
      if (!locked)
         break;

      // Release lock and wait for sim thread
      releaseLock(mem_component);
      _memory_manager->waitForSimThread();
      // Acquire lock and wake up sim thread
      acquireLock(mem_component);
      _memory_manager->wakeUpSimThread();
   }
     
   // Cache line is not locked
   CacheState::Type cstate = l1_cache_line_info.getCState();
   bool cache_hit = true;
   switch (mem_op_type)
   {
   case Core::READ:
      cache_hit = CacheState(cstate).readable();
      break;
   case Core::READ_EX:
   case Core::WRITE:
      cache_hit = CacheState(cstate).writable();
      break;
   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }

   Cache::MissType cache_miss_type = l1_cache->updateMissCounters(address, mem_op_type, !cache_hit);

   return make_pair(!cache_hit, cache_miss_type);
}

void
L1CacheCntlr::accessCache(MemComponent::Type mem_component,
                          Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                          IntPtr ca_address, UInt32 offset,
                          Byte* data_buf, UInt32 data_length)
{
   Cache* l1_cache = getL1Cache(mem_component);
   switch (mem_op_type)
   {
   case Core::READ:
      l1_cache->accessCacheLine(ca_address + offset, Cache::LOAD, data_buf, data_length);
      break;

   case Core::READ_EX:
      assert (lock_signal == Core::LOCK);

      // Read from the L1 cache and lock the L1 cache line
      l1_cache->accessCacheLine(ca_address + offset, Cache::LOAD, data_buf, data_length);
      lockCacheLine(l1_cache, ca_address);

      // Lock the L2 cache line too
      _l2_cache_cntlr->acquireLock();
      _l2_cache_cntlr->lockCacheLine(ca_address);
      _l2_cache_cntlr->releaseLock();
      break;

   case Core::WRITE:
      // Write to the L1 cache and unlock the cache line if the UNLOCK signal is enabled
      l1_cache->accessCacheLine(ca_address + offset, Cache::STORE, data_buf, data_length);
      if (lock_signal == Core::UNLOCK)
         unlockCacheLine(l1_cache, ca_address);
      
      // Write-through L1 cache
      // Write the L2 Cache and unlock the cache line if the UNLOCK signal is enabled
      _l2_cache_cntlr->acquireLock();
      _l2_cache_cntlr->writeCacheLine(ca_address, data_buf, offset, data_length);
      if (lock_signal == Core::UNLOCK)
         _l2_cache_cntlr->unlockCacheLine(ca_address);
      _l2_cache_cntlr->releaseLock();
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }
}

void
L1CacheCntlr::insertCacheLine(MemComponent::Type mem_component, IntPtr address,
                              CacheState::Type cstate, Byte* fill_buf,
                              bool* eviction, IntPtr* evicted_address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   HybridL1CacheLineInfo evicted_cache_line_info;

   HybridL1CacheLineInfo l1_cache_line_info(l1_cache->getTag(address), cstate);
   l1_cache->insertCacheLine(address, &l1_cache_line_info, fill_buf,
                             eviction, evicted_address, &evicted_cache_line_info, NULL);
}

CacheState::Type
L1CacheCntlr::getCacheLineState(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   HybridL1CacheLineInfo l1_cache_line_info;
   // Get cache line state
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   return l1_cache_line_info.getCState();
}

void
L1CacheCntlr::setCacheLineState(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   assert(l1_cache_line_info.getCState() != CacheState::INVALID);

   // Set cache line state
   l1_cache_line_info.setCState(cstate);
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::invalidateCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);
   invalidateCacheLine(l1_cache, address);
}

void
L1CacheCntlr::invalidateCacheLine(Cache* l1_cache, IntPtr address)
{
   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   l1_cache_line_info.invalidate();
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::lockCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);
   lockCacheLine(l1_cache, address);
}

void
L1CacheCntlr::unlockCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);
   unlockCacheLine(l1_cache, address);
}

void
L1CacheCntlr::lockCacheLine(Cache* l1_cache, IntPtr address)
{
   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   l1_cache_line_info.lock();
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::unlockCacheLine(Cache* l1_cache, IntPtr address)
{
   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   l1_cache_line_info.unlock();
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

Cache*
L1CacheCntlr::getL1Cache(MemComponent::Type mem_component)
{
   switch(mem_component)
   {
   case MemComponent::L1_ICACHE:
      return _l1_icache;

   case MemComponent::L1_DCACHE:
      return _l1_dcache;

   default:
      LOG_PRINT_ERROR("Unrecognized Memory Component(%u)", mem_component);
      return NULL;
   }
}

void
L1CacheCntlr::acquireLock(MemComponent::Type mem_component)
{
   switch(mem_component)
   {
   case MemComponent::L1_ICACHE:
      _l1_icache_lock.acquire();
      break;
   
   case MemComponent::L1_DCACHE:
      _l1_dcache_lock.acquire();
      break;
   
   default:
      LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
      break;
   }

}

void
L1CacheCntlr::releaseLock(MemComponent::Type mem_component)
{
   switch(mem_component)
   {
   case MemComponent::L1_ICACHE:
      _l1_icache_lock.release();
      break;
   
   case MemComponent::L1_DCACHE:
      _l1_dcache_lock.release();
      break;
   
   default:
      LOG_PRINT_ERROR("Unrecognized mem_component(%u)", mem_component);
      break;
   }
}

tile_id_t
L1CacheCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
}

UInt32
L1CacheCntlr::getCacheLineSize()
{ 
   return _memory_manager->getCacheLineSize();
}
 
ShmemPerfModel*
L1CacheCntlr::getShmemPerfModel()
{ 
   return _memory_manager->getShmemPerfModel();
}

}
