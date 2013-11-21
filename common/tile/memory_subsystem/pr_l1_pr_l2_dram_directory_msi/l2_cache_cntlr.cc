#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "config.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMSI
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
                           L1CacheCntlr* l1_cache_cntlr,
                           AddressHomeLookup* dram_directory_home_lookup,
                           UInt32 cache_line_size,
                           UInt32 l2_cache_size,
                           UInt32 l2_cache_associativity,
                           UInt32 l2_cache_num_banks,
                           string l2_cache_replacement_policy,
                           UInt32 l2_cache_data_access_cycles,
                           UInt32 l2_cache_tags_access_cycles,
                           string l2_cache_perf_model_type,
                           bool l2_cache_track_miss_types)
   : _memory_manager(memory_manager)
   , _l1_cache_cntlr(l1_cache_cntlr)
   , _dram_directory_home_lookup(dram_directory_home_lookup)
{
   _l2_cache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l2_cache_replacement_policy, l2_cache_size, l2_cache_associativity, cache_line_size);
   _l2_cache_hash_fn_obj = new CacheHashFn(l2_cache_size, l2_cache_associativity, cache_line_size);
   
   _l2_cache = new Cache("L2",
         PR_L1_PR_L2_DRAM_DIRECTORY_MSI,
         Cache::UNIFIED_CACHE,
         L2,
         Cache::WRITE_BACK,
         l2_cache_size,
         l2_cache_associativity,
         cache_line_size,
         l2_cache_num_banks,
         _l2_cache_replacement_policy_obj,
         _l2_cache_hash_fn_obj,
         l2_cache_data_access_cycles,
         l2_cache_tags_access_cycles,
         l2_cache_perf_model_type,
         l2_cache_track_miss_types,
         getShmemPerfModel());
}

L2CacheCntlr::~L2CacheCntlr()
{
   delete _l2_cache;
   delete _l2_cache_replacement_policy_obj;
   delete _l2_cache_hash_fn_obj;
}

void
L2CacheCntlr::invalidateCacheLine(IntPtr address, PrL2CacheLineInfo& l2_cache_line_info)
{
   l2_cache_line_info.invalidate();
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

void
L2CacheCntlr::readCacheLine(IntPtr address, Byte* data_buf)
{
   _l2_cache->accessCacheLine(address, Cache::LOAD, data_buf, getCacheLineSize());
}

void
L2CacheCntlr::writeCacheLine(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length)
{
   _l2_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
}

void
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component)
{
   // Construct meta-data info about l2 cache line
   PrL2CacheLineInfo l2_cache_line_info;
   l2_cache_line_info.setTag(_l2_cache->getTag(address));
   l2_cache_line_info.setCState(cstate);
   l2_cache_line_info.setCachedLoc(mem_component);

   // Evicted line information
   bool eviction;
   IntPtr evicted_address;
   PrL2CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   _l2_cache->insertCacheLine(address, &l2_cache_line_info, fill_buf,
                              &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      LOG_PRINT("Eviction: address(%#lx)", evicted_address);
      invalidateCacheLineInL1(evicted_cache_line_info.getCachedLoc(), evicted_address);

      UInt32 home_node_id = getHome(evicted_address);
      bool eviction_msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());

      if (evicted_cache_line_info.getCState() == CacheState::MODIFIED)
      {
         // Send back the data also
         ShmemMsg msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, getTileId(), evicted_address,
                      writeback_buf, getCacheLineSize(), eviction_msg_modeled);
         _memory_manager->sendMsg(home_node_id, msg);
      }
      else
      {
         LOG_ASSERT_ERROR(evicted_cache_line_info.getCState() == CacheState::SHARED,
               "evicted_address(%#lx), cache state(%u), cached loc(%u)",
               evicted_address, evicted_cache_line_info.getCState(), evicted_cache_line_info.getCachedLoc());
         ShmemMsg msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, getTileId(), evicted_address, eviction_msg_modeled);
         _memory_manager->sendMsg(home_node_id, msg);
      }
   }
}

void
L2CacheCntlr::setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate)
{
   if (mem_component != MemComponent::INVALID)
      _l1_cache_cntlr->setCacheLineState(mem_component, address, cstate);
}

void
L2CacheCntlr::invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address)
{
   if (mem_component != MemComponent::INVALID){
      _l1_cache_cntlr->invalidateCacheLine(mem_component, address);
   }
}

void
L2CacheCntlr::insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address,
                                  CacheState::Type cstate, Byte* fill_buf)
{
   assert(mem_component != MemComponent::INVALID);

   bool eviction;
   IntPtr evicted_address;

   // Insert the Cache Line in L1 Cache
   _l1_cache_cntlr->insertCacheLine(mem_component, address, cstate, fill_buf, &eviction, &evicted_address);

   if (eviction)
   {
      // Clear the Present bit in L2 Cache corresponding to the evicted line
      // Get the cache line info first
      PrL2CacheLineInfo evicted_cache_line_info;
      _l2_cache->getCacheLineInfo(evicted_address, &evicted_cache_line_info);
      
      // Clear the present bit and store the info back
      if (evicted_cache_line_info.getCachedLoc() != mem_component)
      {
         // LOG_PRINT_WARNING("Address(%#lx) removed from (L1-ICACHE)", evicted_address);
         LOG_ASSERT_ERROR(mem_component == MemComponent::L1_ICACHE, "address(%#lx), mem_component(%s), cached_loc(%s)",
                          address, SPELL_MEMCOMP(mem_component), SPELL_MEMCOMP(evicted_cache_line_info.getCachedLoc()));
      }
      else // (evicted_cache_line_info.getCachedLoc() == mem_component)
      {
         evicted_cache_line_info.clearCachedLoc(mem_component);
      }
      _l2_cache->setCacheLineInfo(evicted_address, &evicted_cache_line_info);
   }
}

void
L2CacheCntlr::insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, Byte* fill_buf)
{
   assert(address == _outstanding_shmem_msg.getAddress());
   MemComponent::Type mem_component = _outstanding_shmem_msg.getSenderMemComponent();
  
   // Insert Line in the L2 cache
   insertCacheLine(address, cstate, fill_buf, mem_component);
   
   // Insert Line in the L1 cache
   insertCacheLineInL1(mem_component, address, cstate, fill_buf);
}

pair<bool,Cache::MissType>
L2CacheCntlr::processShmemRequestFromL1Cache(MemComponent::Type mem_component, Core::mem_op_t mem_op_type, IntPtr address)
{
   // L1 Cache synchronization delay
   addSynchronizationCost(mem_component);
   
   LOG_PRINT("processShmemRequestFromL1Cache[Mem Component(%u), Mem Op Type(%u), Address(%#llx)]",
             mem_component, mem_op_type, address);

   PrL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);

   // Get the state associated with the address in the L2 cache
   CacheState::Type cstate = l2_cache_line_info.getCState();

   // Return arguments is a pair
   // (1) Is cache miss? (2) Cache miss type (COLD, CAPACITY, UPGRADE, SHARING)
   pair<bool,Cache::MissType> shmem_request_status_in_l2_cache = operationPermissibleinL2Cache(mem_op_type, address, cstate);
   if (!shmem_request_status_in_l2_cache.first)
   {
      Byte data_buf[getCacheLineSize()];
      // Read the cache line from L2 cache
      readCacheLine(address, data_buf);

      // Insert the cache line in the L1 cache
      insertCacheLineInL1(mem_component, address, cstate, data_buf);
      
      // Set that the cache line in present in the L1 cache in the L2 tags
      if (l2_cache_line_info.getCachedLoc() != MemComponent::INVALID)
      {
         assert(l2_cache_line_info.getCachedLoc() != mem_component);
         assert(cstate == CacheState::SHARED);
         // LOG_PRINT_WARNING("Address(%#lx) cached first in (%s), then in (%s)",
         //                   address, SPELL_MEMCOMP(l2_cache_line_info.getCachedLoc()), SPELL_MEMCOMP(mem_component));
         l2_cache_line_info.setForcedCachedLoc(MemComponent::L1_DCACHE);
      }
      else // (l2_cache_line_info.getCachedLoc() == MemComponent::INVALID)
      {
         l2_cache_line_info.setCachedLoc(mem_component);
      }
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   }
   
   return shmem_request_status_in_l2_cache;
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{

   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   MemComponent::Type sender_mem_component = shmem_msg->getSenderMemComponent();

   assert(shmem_msg->getDataBuf() == NULL);
   assert(shmem_msg->getDataLength() == 0);

   assert(_outstanding_shmem_msg.getAddress() == INVALID_ADDRESS);

   // Set outstanding shmem msg parameters
   _outstanding_shmem_msg.setAddress(address);
   _outstanding_shmem_msg.setSenderMemComponent(sender_mem_component);
   _outstanding_shmem_msg_time = getShmemPerfModel()->getCurrTime();
   
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
}

void
L2CacheCntlr::processExReqFromL1Cache(ShmemMsg* shmem_msg)
{
   // We need to send a request to the Dram Directory Cache
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();

   assert((cstate == CacheState::INVALID) || (cstate == CacheState::SHARED));
   if (cstate == CacheState::SHARED)
   {
      // This will clear the 'Present' bit also
      invalidateCacheLine(address, l2_cache_line_info);
      ShmemMsg msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, getTileId(), address, shmem_msg->isModeled());
      _memory_manager->sendMsg(getHome(address), msg);
   }

   // Send out EX_REQ to DRAM_DIRECTORY
   ShmemMsg msg(ShmemMsg::EX_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, getTileId(), address, shmem_msg->isModeled());
   _memory_manager->sendMsg(getHome(address), msg);
}

void
L2CacheCntlr::processShReqFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   // Send out SH_REQ ro DRAM_DIRECTORY
   ShmemMsg msg(ShmemMsg::SH_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, getTileId(), address, shmem_msg->isModeled());
   _memory_manager->sendMsg(getHome(address), msg);
}

void
L2CacheCntlr::handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // add synchronization cost
   if (sender == getTileId()){
      getShmemPerfModel()->incrCurrTime(_l2_cache->getSynchronizationDelay(DIRECTORY));
   }
   else{
      getShmemPerfModel()->incrCurrTime(_l2_cache->getSynchronizationDelay(NETWORK_MEMORY));
   }

   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
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

   if ((shmem_msg_type == ShmemMsg::EX_REP) || (shmem_msg_type == ShmemMsg::SH_REP))
   {
      LOG_ASSERT_ERROR(_outstanding_shmem_msg_time <= getShmemPerfModel()->getCurrTime(),
                       "Outstanding msg time(%llu), Curr time(%llu)",
                       _outstanding_shmem_msg_time.toNanosec(), getShmemPerfModel()->getCurrTime().toNanosec());
      
      // Reset the clock to the time the request left the tile is miss type is not modeled
      if (!shmem_msg->isModeled())
         getShmemPerfModel()->setCurrTime(_outstanding_shmem_msg_time);

      // Increment the clock by the time taken to update the L2 cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

      // There are no more outstanding memory requests
      _outstanding_shmem_msg.setAddress(INVALID_ADDRESS);
      
      _memory_manager->wakeUpAppThread();
      _memory_manager->waitForAppThread();
   }
}

void
L2CacheCntlr::processExRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   Byte* data_buf = shmem_msg->getDataBuf();
   
   // Insert Cache Line in L1 and L2 Caches
   insertCacheLineInHierarchy(address, CacheState::MODIFIED, data_buf);
}

void
L2CacheCntlr::processShRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   Byte* data_buf = shmem_msg->getDataBuf();

   // Insert Cache Line in L1 and L2 Caches
   insertCacheLineInHierarchy(address, CacheState::SHARED, data_buf);
}

void
L2CacheCntlr::processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   
   CacheState::Type cstate = l2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::SHARED);
  
      // Update Shared memory performance counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // add synchronization delay: L2->L1
      _l1_cache_cntlr->addSynchronizationCost(l2_cache_line_info.getCachedLoc(), L2_CACHE);

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(l2_cache_line_info.getCachedLoc(), address);

      // add synchronization delay: L1->L2
      addSynchronizationCost(l2_cache_line_info.getCachedLoc());

      // Invalidate the line in the L2 cache also
      invalidateCacheLine(address, l2_cache_line_info);


      // Send out INV_REP to DRAM_DIRECTORY
      ShmemMsg msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, shmem_msg->getRequester(), address, shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
   }
}

void
L2CacheCntlr::processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();
   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::MODIFIED);
      
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // add synchronization delay: L2->L1
      _l1_cache_cntlr->addSynchronizationCost(l2_cache_line_info.getCachedLoc(), L2_CACHE);

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(l2_cache_line_info.getCachedLoc(), address);

      // add synchronization delay: L1->L2
      addSynchronizationCost(l2_cache_line_info.getCachedLoc());

      // Write-back the line
      Byte data_buf[getCacheLineSize()];
      readCacheLine(address, data_buf);

      // Invalidate the line
      invalidateCacheLine(address, l2_cache_line_info);

      // Send FLUSH_REP to DRAM_DIRECTORY
      ShmemMsg msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, shmem_msg->getRequester(), address,
                   data_buf, getCacheLineSize(), shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
   }
}

void
L2CacheCntlr::processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::MODIFIED);
 
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // add synchronization delay: L2->L1
      _l1_cache_cntlr->addSynchronizationCost(l2_cache_line_info.getCachedLoc(), L2_CACHE);

      // Set the Appropriate Cache State in L1 also
      setCacheLineStateInL1(l2_cache_line_info.getCachedLoc(), address, CacheState::SHARED);

      // add synchronization delay: L1->L2
      addSynchronizationCost(l2_cache_line_info.getCachedLoc());

      // Write-Back the line
      Byte data_buf[getCacheLineSize()];
      readCacheLine(address, data_buf);
      
      // Set the cache line state to SHARED
      l2_cache_line_info.setCState(CacheState::SHARED);
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

      // Send WB_REP to DRAM_DIRECTORY
      ShmemMsg msg(ShmemMsg::WB_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY, shmem_msg->getRequester(), address,
                   data_buf, getCacheLineSize(), shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
   }
}

pair<bool,Cache::MissType>
L2CacheCntlr::operationPermissibleinL2Cache(Core::mem_op_t mem_op_type, IntPtr address, CacheState::Type cstate)
{
   bool cache_hit = false;

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
      LOG_PRINT_ERROR("Unsupported Mem Op Type(%u)", mem_op_type);
      break;
   }

   Cache::MissType cache_miss_type = _l2_cache->updateMissCounters(address, mem_op_type, !cache_hit);
   return make_pair(!cache_hit, cache_miss_type);
}

tile_id_t
L2CacheCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
}

UInt32
L2CacheCntlr::getCacheLineSize()
{ 
   return _memory_manager->getCacheLineSize();
}
 
ShmemPerfModel*
L2CacheCntlr::getShmemPerfModel()
{ 
   return _memory_manager->getShmemPerfModel();
}

void
L2CacheCntlr::addSynchronizationCost(MemComponent::Type mem_component)
{
   if (mem_component != MemComponent::INVALID){
      module_t module = DVFSManager::convertToModule(mem_component);
      getShmemPerfModel()->incrCurrTime(_l2_cache->getSynchronizationDelay(module));
   }
}

}
