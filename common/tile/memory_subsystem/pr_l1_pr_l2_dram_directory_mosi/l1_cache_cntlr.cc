#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h" 
#include "memory_manager.h"
#include "config.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

L1CacheCntlr::L1CacheCntlr(MemoryManager* memory_manager,
                           UInt32 cache_line_size,
                           UInt32 L1_icache_size,
                           UInt32 L1_icache_associativity,
                           UInt32 L1_icache_num_banks,
                           string L1_icache_replacement_policy,
                           UInt32 L1_icache_data_access_cycles,
                           UInt32 L1_icache_tags_access_cycles,
                           string L1_icache_perf_model_type,
                           bool L1_icache_track_miss_types,
                           UInt32 L1_dcache_size,
                           UInt32 L1_dcache_associativity,
                           UInt32 L1_dcache_num_banks,
                           string L1_dcache_replacement_policy,
                           UInt32 L1_dcache_data_access_cycles,
                           UInt32 L1_dcache_tags_access_cycles,
                           string L1_dcache_perf_model_type,
                           bool L1_dcache_track_miss_types)
   : _memory_manager(memory_manager)
   , _L2_cache_cntlr(NULL)
{
   _L1_icache_replacement_policy_obj = 
      CacheReplacementPolicy::create(L1_icache_replacement_policy, L1_icache_size, L1_icache_associativity, cache_line_size);
   _L1_dcache_replacement_policy_obj = 
      CacheReplacementPolicy::create(L1_dcache_replacement_policy, L1_dcache_size, L1_dcache_associativity, cache_line_size);
   _L1_icache_hash_fn_obj = new CacheHashFn(L1_icache_size, L1_icache_associativity, cache_line_size);
   _L1_dcache_hash_fn_obj = new CacheHashFn(L1_dcache_size, L1_dcache_associativity, cache_line_size);
   
   _L1_icache = new Cache("L1-I",
         PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
         Cache::INSTRUCTION_CACHE,
         L1,
         Cache::UNDEFINED_WRITE_POLICY,
         L1_icache_size,
         L1_icache_associativity, 
         cache_line_size,
         L1_icache_num_banks,
         _L1_icache_replacement_policy_obj,
         _L1_icache_hash_fn_obj,
         L1_icache_data_access_cycles,
         L1_icache_tags_access_cycles,
         L1_icache_perf_model_type,
         L1_icache_track_miss_types,
         getShmemPerfModel());
   _L1_dcache = new Cache("L1-D",
         PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
         Cache::DATA_CACHE,
         L1,
         Cache::WRITE_THROUGH,
         L1_dcache_size,
         L1_dcache_associativity,
         cache_line_size,
         L1_dcache_num_banks,
         _L1_dcache_replacement_policy_obj,
         _L1_dcache_hash_fn_obj,
         L1_dcache_data_access_cycles,
         L1_dcache_tags_access_cycles,
         L1_dcache_perf_model_type,
         L1_dcache_track_miss_types,
         getShmemPerfModel());
}

L1CacheCntlr::~L1CacheCntlr()
{
   delete _L1_icache;
   delete _L1_dcache;
   delete _L1_icache_replacement_policy_obj;
   delete _L1_dcache_replacement_policy_obj;
   delete _L1_icache_hash_fn_obj;
   delete _L1_dcache_hash_fn_obj;
}      

void
L1CacheCntlr::setL2CacheCntlr(L2CacheCntlr* L2_cache_cntlr)
{
   _L2_cache_cntlr = L2_cache_cntlr;
}

bool
L1CacheCntlr::processMemOpFromCore(MemComponent::Type mem_component,
                                   Core::lock_signal_t lock_signal,
                                   Core::mem_op_t mem_op_type, 
                                   IntPtr ca_address, UInt32 offset,
                                   Byte* data_buf, UInt32 data_length,
                                   bool modeled)
{
   LOG_PRINT("processMemOpFromCore(), lock_signal(%u), mem_op_type(%u), ca_address(%#llx)",
             lock_signal, mem_op_type, ca_address);

   bool L1_cache_hit = true;
   UInt32 access_num = 0;

   // Core synchronization delay
   getShmemPerfModel()->incrCurrTime(getL1Cache(mem_component)->getSynchronizationDelay(CORE));

   while(1)
   {
      access_num ++;
      LOG_ASSERT_ERROR((access_num == 1) || (access_num == 2),
            "Error: access_num(%u)", access_num);

      // Wake up the network thread after acquiring the lock
      if (access_num == 2)
      {
         _memory_manager->wakeUpSimThread();
      }

      if (operationPermissibleinL1Cache(mem_component, ca_address, mem_op_type, access_num))
      {
         // Increment Shared Mem Perf model current time 
         // L1 Cache
         _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_DATA_AND_TAGS);

         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);
         return L1_cache_hit;
      }

      _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_TAGS);

      // The memory request misses in the L1 cache
      L1_cache_hit = false;

      if (lock_signal == Core::UNLOCK)
         LOG_PRINT_ERROR("Expected to find address(%#lx) in L1 Cache", ca_address);

      pair<bool,Cache::MissType> L2_cache_miss_info = _L2_cache_cntlr->processShmemRequestFromL1Cache(mem_component, mem_op_type, ca_address);
      bool L2_cache_miss = L2_cache_miss_info.first;

      if (!L2_cache_miss)
      {

         // L2 Cache synchronization delay 
         getShmemPerfModel()->incrCurrTime(getL1Cache(mem_component)->getSynchronizationDelay(L2_CACHE));

         // Increment Shared Mem Perf model current time
         // L2 Cache
         _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);
         // L1 Cache
         _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_DATA_AND_TAGS);

         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);

         return false;
      }

      // Increment shared mem perf model current time 
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);

      // Send out a request to the network thread for the cache data
      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());

      ShmemMsg::Type shmem_msg_type = getShmemMsgType(mem_op_type);
      ShmemMsg shmem_msg(shmem_msg_type, mem_component, MemComponent::L2_CACHE,
                         getTileId(), INVALID_TILE_ID, false, ca_address, msg_modeled);
      _memory_manager->sendMsg(getTileId(), shmem_msg);

      _memory_manager->waitForSimThread();

      // L2 Cache synchronization delay 
      getShmemPerfModel()->incrCurrTime(getL1Cache(mem_component)->getSynchronizationDelay(L2_CACHE));
   }

   LOG_PRINT_ERROR("Should not reach here");
   return false;
}

void
L1CacheCntlr::accessCache(MemComponent::Type mem_component,
                          Core::mem_op_t mem_op_type, IntPtr ca_address, UInt32 offset,
                          Byte* data_buf, UInt32 data_length)
{
   Cache* L1_cache = getL1Cache(mem_component);
   switch (mem_op_type)
   {
   case Core::READ:
   case Core::READ_EX:
      L1_cache->accessCacheLine(ca_address + offset, Cache::LOAD, data_buf, data_length);
      break;

   case Core::WRITE:
      L1_cache->accessCacheLine(ca_address + offset, Cache::STORE, data_buf, data_length);
      // Write-through cache - Write the L2 Cache also
      _L2_cache_cntlr->writeCacheLine(ca_address, offset, data_buf, data_length);
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }
}

bool
L1CacheCntlr::operationPermissibleinL1Cache(MemComponent::Type mem_component, 
      IntPtr address, Core::mem_op_t mem_op_type,
      UInt32 access_num)
{
   bool cache_hit = false;
   CacheState::Type cstate = getCacheLineState(mem_component, address);
   
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
      LOG_PRINT_ERROR("Unsupported mem_op_type: %u", mem_op_type);
      break;
   }

   if (access_num == 1)
   {
      // Update the Cache Counters
      getL1Cache(mem_component)->updateMissCounters(address, mem_op_type, !cache_hit);
   }

   return cache_hit;
}

void
L1CacheCntlr::insertCacheLine(MemComponent::Type mem_component,
                              IntPtr address, CacheState::Type cstate, Byte* fill_buf,
                              bool* eviction, PrL1CacheLineInfo* evicted_cache_line_info, IntPtr* evicted_address)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   PrL1CacheLineInfo L1_cache_line_info(L1_cache->getTag(address), cstate);

   L1_cache->insertCacheLine(address, &L1_cache_line_info, fill_buf,
                             eviction, evicted_address, evicted_cache_line_info, NULL);
}

CacheState::Type
L1CacheCntlr::getCacheLineState(MemComponent::Type mem_component, IntPtr address)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   PrL1CacheLineInfo L1_cache_line_info;
   // Get cache line state
   L1_cache->getCacheLineInfo(address, &L1_cache_line_info);
   return L1_cache_line_info.getCState();
}

void
L1CacheCntlr::setCacheLineState(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   PrL1CacheLineInfo L1_cache_line_info;
   L1_cache->getCacheLineInfo(address, &L1_cache_line_info);
   assert(L1_cache_line_info.getCState() != CacheState::INVALID);

   // Set cache line state
   L1_cache_line_info.setCState(cstate);
   L1_cache->setCacheLineInfo(address, &L1_cache_line_info);
}

void
L1CacheCntlr::invalidateCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   PrL1CacheLineInfo L1_cache_line_info;
   L1_cache->getCacheLineInfo(address, &L1_cache_line_info);
   // Invalidate cache line
   L1_cache_line_info.invalidate();
   L1_cache->setCacheLineInfo(address, &L1_cache_line_info);
}

void
L1CacheCntlr::addSynchronizationCost(MemComponent::Type mem_component, module_t module)
{

   if (mem_component != MemComponent::INVALID){
      getShmemPerfModel()->incrCurrTime(getL1Cache(mem_component)->getSynchronizationDelay(module));
   }
}

ShmemMsg::Type
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
      return ShmemMsg::INVALID;
   }
}

Cache*
L1CacheCntlr::getL1Cache(MemComponent::Type mem_component)
{
   switch(mem_component)
   {
   case MemComponent::L1_ICACHE:
      return _L1_icache;

   case MemComponent::L1_DCACHE:
      return _L1_dcache;

   default:
      LOG_PRINT_ERROR("Unrecognized Memory Component(%s)", SPELL_MEMCOMP(mem_component));
      return NULL;
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
