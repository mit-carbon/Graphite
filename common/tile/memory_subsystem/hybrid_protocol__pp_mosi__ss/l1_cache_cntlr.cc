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
         L1,
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
                                   Core::mem_op_t mem_op_type, 
                                   Core::lock_signal_t lock_signal,
                                   IntPtr address, Byte* data_buf,
                                   UInt32 offset, UInt32 data_length,
                                   bool modeled)
{
   // It would be great if we had a way to predict the cache coherence protocol by
   // looking at the instruction address (EIP). This would imply that we need a way to roll-back
   // our actions if the prediction was wrong.
   LOG_PRINT("processMemOpFromCore(), lock_signal(%u), mem_op_type(%u), address(%#lx), offset(%u), data_length(%u)",
             lock_signal, mem_op_type, address, offset, data_length);

   // Acquire the lock for the cache hierarchy on the tile
   _memory_manager->acquireLock();

   HybridL1CacheLineInfo l1_cache_line_info;
   pair<bool,Cache::MissType> l1_cache_status = getMemOpStatusInL1Cache(mem_component, address,
                                                                        mem_op_type, lock_signal, l1_cache_line_info);
   bool l1_cache_hit = !l1_cache_status.first;
   if (l1_cache_hit)
   {
      // Incr cycle count
      getMemoryManager()->incrCycleCount(mem_component, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

      accessCache(mem_component, lock_signal, mem_op_type, address, offset, data_buf, data_length, l1_cache_line_info);
      _memory_manager->releaseLock();
      if ( (mem_op_type == Core::WRITE) && (lock_signal == Core::UNLOCK) )
         _memory_manager->wakeUpThreadAfterCacheLineUnlock(ShmemPerfModel::_SIM_THREAD, address);
      return true;
   }

   // Incr cycle count
   getMemoryManager()->incrCycleCount(mem_component, CachePerfModel::ACCESS_CACHE_TAGS);
   
   bool l2_cache_hit = _l2_cache_cntlr->processMemOpFromL1Cache(mem_component, mem_op_type, lock_signal,
                                                                address, data_buf, offset, data_length,
                                                                modeled);
   _memory_manager->releaseLock();

   // Wait until the data is fetched
   if (l2_cache_hit)
   {
      LOG_ASSERT_ERROR(!( (mem_op_type == Core::WRITE) && (lock_signal == Core::UNLOCK) ), "mem_op_type(WRITE), lock_signal(UNLOCK)");
   }
   else // (!l2_cache_hit)
   {
      _memory_manager->waitForSimThread();
   }
   return false;
}


pair<bool,Cache::MissType>
L1CacheCntlr::getMemOpStatusInL1Cache(MemComponent::Type mem_component, IntPtr address,
                                      Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                      HybridL1CacheLineInfo& l1_cache_line_info)
{
   Cache* l1_cache = getL1Cache(mem_component);
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);

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
                          IntPtr address, UInt32 offset,
                          Byte* data_buf, UInt32 data_length,
                          HybridL1CacheLineInfo& l1_cache_line_info)
{
   CacheState::Type cstate = l1_cache_line_info.getCState();
   Cache* l1_cache = getL1Cache(mem_component);
      
   switch (mem_op_type)
   {
   case Core::READ:
      l1_cache->accessCacheLine(address + offset, Cache::LOAD, data_buf, data_length);
      break;

   case Core::READ_EX:
      LOG_ASSERT_ERROR (lock_signal == Core::LOCK, "lock_signal(%u)", lock_signal);

      // Read from the L1 cache and lock the L1 cache line
      l1_cache->accessCacheLine(address + offset, Cache::LOAD, data_buf, data_length);

      // Lock the L2 cache line too
      _l2_cache_cntlr->lockCacheLine(address);
      break;

   case Core::WRITE:
      // Write to the L1 cache and unlock the cache line if the UNLOCK signal is enabled
      l1_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
      if (cstate == CacheState::EXCLUSIVE)
         l1_cache_line_info.setCState(CacheState::MODIFIED);
      
      // Write-through L1 cache
      // Write the L2 Cache and unlock the cache line if the UNLOCK signal is enabled
      _l2_cache_cntlr->writeCacheLine(address, data_buf, offset, data_length);
      if (cstate == CacheState::EXCLUSIVE)
         _l2_cache_cntlr->setCacheLineState(address, CacheState::MODIFIED);
      if (lock_signal == Core::UNLOCK)
         _l2_cache_cntlr->unlockCacheLine(address);
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }
   
   // Increment utilization
   l1_cache_line_info.incrUtilization();
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::assertL1CacheReady(Core::mem_op_t mem_op_type, CacheState::Type cstate)
{
   switch (mem_op_type)
   {
   case Core::READ:
      LOG_ASSERT_ERROR(CacheState(cstate).readable(), "MemOpType(%s), CState(%s)",
                       SPELL_MEMOP(mem_op_type), SPELL_CSTATE(cstate));
      break;
   case Core::READ_EX:
   case Core::WRITE:
      LOG_ASSERT_ERROR(CacheState(cstate).writable(), "MemOpType(%s), CState(%s)",
                       SPELL_MEMOP(mem_op_type), SPELL_CSTATE(cstate));
      break;
   default:
      LOG_PRINT_ERROR("Unrecgonized mem op type(%s)", SPELL_MEMOP(mem_op_type));
      break;      
   }
}

void
L1CacheCntlr::processMemOpL1CacheReady(MemComponent::Type mem_component,
                                       Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                       IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length)
{
   Cache* l1_cache = getL1Cache(mem_component);
   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);

   // Check if L1 cache is ready
   assertL1CacheReady(mem_op_type, l1_cache_line_info.getCState());
   
   if (mem_op_type == Core::WRITE)
   {
      l1_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
      l1_cache_line_info.setCState(CacheState::MODIFIED);
   }
   
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

void
L1CacheCntlr::writeCacheLine(Cache* l1_cache, IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length)
{
   data_length = (data_length == UINT32_MAX_) ? getCacheLineSize() : data_length;
   l1_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
}

void
L1CacheCntlr::insertCacheLine(MemComponent::Type mem_component, IntPtr address,
                              CacheState::Type cstate, Byte* fill_buf,
                              bool& eviction, IntPtr& evicted_address, UInt32& evicted_line_utilization)
{
   LOG_PRINT("insertCacheLine[Address(%#lx), CState(%s), Fill Buf(%p), MemComponent(%s)] start",
             address, SPELL_CSTATE(cstate), fill_buf, MemComponent::getName(mem_component).c_str());

   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   HybridL1CacheLineInfo evicted_cache_line_info;

   HybridL1CacheLineInfo l1_cache_line_info(l1_cache->getTag(address), cstate);
   l1_cache->insertCacheLine(address, &l1_cache_line_info, fill_buf,
                             &eviction, &evicted_address, &evicted_cache_line_info, NULL);
   evicted_line_utilization = evicted_cache_line_info.getUtilization();

   LOG_PRINT("insertCacheLine[Address(%#lx), CState(%s), Fill Buf(%p), MemComponent(%s)] end",
             address, SPELL_CSTATE(cstate), fill_buf, MemComponent::getName(mem_component).c_str());
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
   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   assert (l1_cache_line_info.isValid());
   l1_cache_line_info.invalidate();
   l1_cache->setCacheLineInfo(address, &l1_cache_line_info);
}

UInt32
L1CacheCntlr::getCacheLineUtilization(MemComponent::Type mem_component, IntPtr address)
{
   Cache* l1_cache = getL1Cache(mem_component);
   assert(l1_cache);

   HybridL1CacheLineInfo l1_cache_line_info;
   l1_cache->getCacheLineInfo(address, &l1_cache_line_info);
   return l1_cache_line_info.getUtilization();
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

tile_id_t
L1CacheCntlr::getTileID()
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
