#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "config.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
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
                           bool L2_cache_track_miss_types)
   : _memory_manager(memory_manager)
   , _L1_cache_cntlr(L1_cache_cntlr)
   , _dram_directory_home_lookup(dram_directory_home_lookup)
   , _enabled(false)
{
   _L2_cache_replacement_policy_obj = 
      CacheReplacementPolicy::create(L2_cache_replacement_policy, L2_cache_size, L2_cache_associativity, cache_line_size);
   _L2_cache_hash_fn_obj = new CacheHashFn(L2_cache_size, L2_cache_associativity, cache_line_size);
   
   _L2_cache = new Cache("L2",
         PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
         Cache::UNIFIED_CACHE,
         L2,
         Cache::WRITE_BACK,
         L2_cache_size,
         L2_cache_associativity,
         cache_line_size,
         L2_cache_num_banks,
         _L2_cache_replacement_policy_obj,
         _L2_cache_hash_fn_obj,
         L2_cache_data_access_cycles,
         L2_cache_tags_access_cycles,
         L2_cache_perf_model_type,
         L2_cache_track_miss_types,
         getShmemPerfModel());

   initializeEvictionCounters();
   initializeInvalidationCounters();
}

L2CacheCntlr::~L2CacheCntlr()
{
   delete _L2_cache;
   delete _L2_cache_replacement_policy_obj;
   delete _L2_cache_hash_fn_obj;
}

void
L2CacheCntlr::initializeEvictionCounters()
{
   // Eviction counters
   _total_evictions = 0;
   _total_dirty_evictions_exreq = 0;
   _total_clean_evictions_exreq = 0;
   _total_dirty_evictions_shreq = 0;
   _total_clean_evictions_shreq = 0;
}

void
L2CacheCntlr::initializeInvalidationCounters()
{
   _total_invalidations = 0;
}

void
L2CacheCntlr::invalidateCacheLine(IntPtr address, PrL2CacheLineInfo& L2_cache_line_info)
{
   L2_cache_line_info.invalidate();
   _L2_cache->setCacheLineInfo(address, &L2_cache_line_info);
}

void
L2CacheCntlr::readCacheLine(IntPtr address, Byte* data_buf)
{
  _L2_cache->accessCacheLine(address, Cache::LOAD, data_buf, getCacheLineSize());
}

void
L2CacheCntlr::writeCacheLine(IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length)
{
   _L2_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
}

void
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component)
{
   // Construct meta-data info about L2 cache line
   PrL2CacheLineInfo L2_cache_line_info(_L2_cache->getTag(address), cstate, mem_component);

   // Evicted Line Information
   bool eviction;
   IntPtr evicted_address;
   PrL2CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   _L2_cache->insertCacheLine(address, &L2_cache_line_info, fill_buf,
                              &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      assert(evicted_cache_line_info.isValid());
      LOG_PRINT("Eviction: address(%#lx)", evicted_address);
          
      LOG_PRINT("Address(%#lx), Cached Loc(%s)", evicted_address, SPELL_MEMCOMP(evicted_cache_line_info.getCachedLoc())); 
      CacheState::Type evicted_cstate = evicted_cache_line_info.getCState();
      // Update eviction counters so as to track clean and dirty evictions
      updateEvictionCounters(cstate, evicted_cstate);

      // Invalidate the cache line in L1-I/L1-D + get utilization
      invalidateCacheLineInL1(evicted_cache_line_info.getCachedLoc(), evicted_address);

      UInt32 home_node_id = getHome(evicted_address);

      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());

      if ((evicted_cstate == CacheState::MODIFIED) || (evicted_cstate == CacheState::OWNED))
      {
         // Send back the data also
         ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                                , getTileId(), INVALID_TILE_ID, false, evicted_address
                                , writeback_buf, getCacheLineSize(), msg_modeled
                                );
         _memory_manager->sendMsg(home_node_id, send_shmem_msg);
      }
      else
      {
         LOG_ASSERT_ERROR(evicted_cache_line_info.getCState() == CacheState::SHARED,
                          "evicted_address(%#lx), evicted_cstate(%u), cached_loc(%u)",
                          evicted_address, evicted_cache_line_info.getCState(), evicted_cache_line_info.getCachedLoc());
         
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                                , getTileId(), INVALID_TILE_ID, false, evicted_address
                                ,  msg_modeled
                                );
         _memory_manager->sendMsg(home_node_id, send_shmem_msg);
      }
   }
}

void
L2CacheCntlr::setCacheLineStateInL1(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate)
{
   if (mem_component != MemComponent::INVALID)
   {
      _L1_cache_cntlr->setCacheLineState(mem_component, address, cstate);
   }
}

void
L2CacheCntlr::invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address)
{
   if (mem_component != MemComponent::INVALID)
   {
      _L1_cache_cntlr->invalidateCacheLine(mem_component, address);
   }
}

void
L2CacheCntlr::insertCacheLineInL1(MemComponent::Type mem_component, IntPtr address,
                                  CacheState::Type cstate, Byte* fill_buf)
{
   assert(mem_component != MemComponent::INVALID);

   bool eviction;
   PrL1CacheLineInfo evicted_cache_line_info;
   IntPtr evicted_address;

   // Insert the Cache Line in L1 Cache
   _L1_cache_cntlr->insertCacheLine(mem_component, address, cstate, fill_buf,
                                    &eviction, &evicted_cache_line_info, &evicted_address);

   if (eviction)
   {
      // Evicted cache line is valid
      assert(evicted_cache_line_info.isValid());

      // Clear the Present bit in L2 Cache corresponding to the evicted line
      // Get the cache line info first
      PrL2CacheLineInfo L2_cache_line_info;
      _L2_cache->getCacheLineInfo(evicted_address, &L2_cache_line_info);
      
      // Clear the present bit and store the info back
      if (L2_cache_line_info.getCachedLoc() != mem_component)
      {
         // LOG_PRINT_WARNING("Address(%#lx) removed from (L1-ICACHE)", evicted_address);
         LOG_ASSERT_ERROR(mem_component == MemComponent::L1_ICACHE, "address(%#lx), mem_component(%s), cached_loc(%s)",
                          address, SPELL_MEMCOMP(mem_component), SPELL_MEMCOMP(L2_cache_line_info.getCachedLoc()));
      }
      else // (L2_cache_line_info.getCachedLoc() == mem_component)
      {
         L2_cache_line_info.clearCachedLoc(mem_component);
      }
      
      // Update the cache with the new line info
      _L2_cache->setCacheLineInfo(evicted_address, &L2_cache_line_info);
   }
}

void
L2CacheCntlr::insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, Byte* fill_buf)
{
   LOG_ASSERT_ERROR(address == _outstanding_shmem_msg.getAddress(), 
                    "Got Address(%#lx), Expected Address(%#lx) from Directory",
                    address, _outstanding_shmem_msg.getAddress());
   
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

   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
   CacheState::Type L2_cstate = L2_cache_line_info.getCState();
   
   pair<bool,Cache::MissType> shmem_request_status_in_L2_cache = operationPermissibleinL2Cache(mem_op_type, address, L2_cstate);
   if (!shmem_request_status_in_L2_cache.first)
   {
      Byte data_buf[getCacheLineSize()];
      
      // Read the cache line from L2 cache
      readCacheLine(address, data_buf);

      // Insert the cache line in the L1 cache
      insertCacheLineInL1(mem_component, address, L2_cstate, data_buf);
      
      // Set that the cache line in present in the L1 cache in the L2 tags
      if (L2_cache_line_info.getCachedLoc() != MemComponent::INVALID)
      {
         assert(L2_cache_line_info.getCachedLoc() != mem_component);
         assert(L2_cstate == CacheState::SHARED);
         // LOG_PRINT_WARNING("Address(%#lx) cached first in (%s), then in (%s)",
         //                   address, SPELL_MEMCOMP(L2_cache_line_info.getCachedLoc()), SPELL_MEMCOMP(mem_component));
         L2_cache_line_info.setForcedCachedLoc(MemComponent::L1_DCACHE);
      }
      else // (L2_cache_line_info.getCachedLoc() == MemComponent::INVALID)
      {
         L2_cache_line_info.setCachedLoc(mem_component);
      }
      _L2_cache->setCacheLineInfo(address, &L2_cache_line_info);
   }
   
   return shmem_request_status_in_L2_cache;
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   assert(shmem_msg->getDataBuf() == NULL);
   assert(shmem_msg->getDataLength() == 0);

   LOG_ASSERT_ERROR(_outstanding_shmem_msg.getAddress() == INVALID_ADDRESS, 
                    "_outstanding_address(%#llx)", _outstanding_shmem_msg.getAddress());

   // Set Outstanding shmem req parameters
   _outstanding_shmem_msg = *shmem_msg;
   _outstanding_shmem_msg_time = getShmemPerfModel()->getCurrTime();

   ShmemMsg send_shmem_msg(shmem_msg_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                           getTileId(), INVALID_TILE_ID, false, address, shmem_msg->isModeled()); 
   _memory_manager->sendMsg(getHome(address), send_shmem_msg);
}

void
L2CacheCntlr::handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // add synchronization cost
   if (sender == getTileId()){
      getShmemPerfModel()->incrCurrTime(_L2_cache->getSynchronizationDelay(DIRECTORY));
   }
   else{
      getShmemPerfModel()->incrCurrTime(_L2_cache->getSynchronizationDelay(NETWORK_MEMORY));
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
   case ShmemMsg::UPGRADE_REP:
      processUpgradeRepFromDramDirectory(sender, shmem_msg);
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
   case ShmemMsg::INV_FLUSH_COMBINED_REQ:
      processInvFlushCombinedReqFromDramDirectory(sender, shmem_msg);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized msg type: %u", shmem_msg_type);
      break;
   }
   
   if ((shmem_msg_type == ShmemMsg::EX_REP) || (shmem_msg_type == ShmemMsg::SH_REP) || (shmem_msg_type == ShmemMsg::UPGRADE_REP))
   {
      assert(_outstanding_shmem_msg_time <= getShmemPerfModel()->getCurrTime());
      
      // Reset the clock to the time the request left the tile is miss type is not modeled
      LOG_ASSERT_ERROR(_outstanding_shmem_msg.isModeled() == shmem_msg->isModeled(), "Request(%s), Response(%s)",
                       _outstanding_shmem_msg.isModeled() ? "MODELED" : "UNMODELED",
                       shmem_msg->isModeled() ? "MODELED" : "UNMODELED");

      if (!_outstanding_shmem_msg.isModeled())
         getShmemPerfModel()->setCurrTime(_outstanding_shmem_msg_time);

      // Increment the clock by the time taken to update the L2 cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

      // There are no more outstanding memory requests
      _outstanding_shmem_msg = ShmemMsg();
      
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
L2CacheCntlr::processUpgradeRepFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   // Just change state from (SHARED,OWNED) -> MODIFIED
   // In L2
   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);

   // Get cache line state
   __attribute__((unused)) CacheState::Type L2_cstate = L2_cache_line_info.getCState();
   LOG_ASSERT_ERROR((L2_cstate == CacheState::SHARED) || (L2_cstate == CacheState::OWNED), 
                    "Address(%#lx), State(%u)", address, L2_cstate);
   
   L2_cache_line_info.setCState(CacheState::MODIFIED);

   // In L1
   LOG_ASSERT_ERROR(address == _outstanding_shmem_msg.getAddress(), 
                    "Got Address(%#lx), Expected Address(%#lx) from Directory",
                    address, _outstanding_shmem_msg.getAddress());

   MemComponent::Type mem_component = _outstanding_shmem_msg.getSenderMemComponent();
   assert(mem_component == MemComponent::L1_DCACHE);
   
   if (L2_cache_line_info.getCachedLoc() == MemComponent::INVALID)
   {
      Byte data_buf[getCacheLineSize()];
      readCacheLine(address, data_buf);

      // Insert cache line in L1
      insertCacheLineInL1(mem_component, address, CacheState::MODIFIED, data_buf);
      // Set cached loc
      L2_cache_line_info.setCachedLoc(mem_component);
   }
   else // (L2_cache_line_info.getCachedLoc() != MemComponent::INVALID)
   {
      setCacheLineStateInL1(mem_component, address, CacheState::MODIFIED);
   }
   
   // Set the meta-date in the L2 cache   
   _L2_cache->setCacheLineInfo(address, &L2_cache_line_info);
}

void
L2CacheCntlr::processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
   CacheState::Type cstate = L2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::SHARED);

      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // SHARED -> INVALID 

      LOG_PRINT("Address(%#lx), Cached Loc(%s)", address, SPELL_MEMCOMP(L2_cache_line_info.getCachedLoc()));
      updateInvalidationCounters();

      // add synchronization delay: L2->L1
      _L1_cache_cntlr->addSynchronizationCost(L2_cache_line_info.getCachedLoc(), L2_CACHE);
     
      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(L2_cache_line_info.getCachedLoc(), address);

      // add synchronization delay: L1->L2
      addSynchronizationCost(L2_cache_line_info.getCachedLoc());
      
      // Invalidate the line in the L2 cache
      invalidateCacheLine(address, L2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                             , shmem_msg->getRequester(), INVALID_TILE_ID, shmem_msg->isReplyExpected(), address
                             , shmem_msg->isModeled()
                             );
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 shmem_msg->getRequester(), INVALID_TILE_ID, true, address,
                                 shmem_msg->isModeled());
         _memory_manager->sendMsg(sender, send_shmem_msg);
      }
   }
}

void
L2CacheCntlr::processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
   CacheState::Type cstate = L2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // (MODIFIED, OWNED, SHARED) -> INVALID
     
      LOG_PRINT("Address(%#lx), Cached Loc(%s)", address, SPELL_MEMCOMP(L2_cache_line_info.getCachedLoc()));
      updateInvalidationCounters();

      // add synchronization delay: L2->L1
      _L1_cache_cntlr->addSynchronizationCost(L2_cache_line_info.getCachedLoc(), L2_CACHE);

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(L2_cache_line_info.getCachedLoc(), address);

      // add synchronization delay: L1->L2
      addSynchronizationCost(L2_cache_line_info.getCachedLoc());

      // Flush the line
      Byte data_buf[getCacheLineSize()];
      // First get the cache line to write it back
      readCacheLine(address, data_buf);
   
      // Invalidate the cache line in the L2 cache
      invalidateCacheLine(address, L2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                             , shmem_msg->getRequester(), INVALID_TILE_ID, shmem_msg->isReplyExpected(), address
                             , data_buf, getCacheLineSize(), shmem_msg->isModeled()
                             );
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 shmem_msg->getRequester(), INVALID_TILE_ID, true, address,
                                 shmem_msg->isModeled());
         _memory_manager->sendMsg(sender, send_shmem_msg);
      }
   }
}

void
L2CacheCntlr::processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   assert(!shmem_msg->isReplyExpected());

   IntPtr address = shmem_msg->getAddress();

   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
   CacheState::Type cstate = L2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_TAGS);

      // MODIFIED -> OWNED, OWNED -> OWNED, SHARED -> SHARED
      CacheState::Type new_cstate = (cstate == CacheState::MODIFIED) ? CacheState::OWNED : cstate;

      // add synchronization delay: L2->L1
      _L1_cache_cntlr->addSynchronizationCost(L2_cache_line_info.getCachedLoc(), L2_CACHE);
      
      // Set the Appropriate Cache State in the L1 cache
      setCacheLineStateInL1(L2_cache_line_info.getCachedLoc(), address, new_cstate);

      // add synchronization delay: L1->L2
      addSynchronizationCost(L2_cache_line_info.getCachedLoc());

      // Write-Back the line
      Byte data_buf[getCacheLineSize()];
      // Read the cache line into a local buffer
      readCacheLine(address, data_buf);
      // set the state to OWNED/SHARED
      L2_cache_line_info.setCState(new_cstate);

      // Write-back the new state in the L2 cache
      _L2_cache->setCacheLineInfo(address, &L2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::WB_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                             , shmem_msg->getRequester(), INVALID_TILE_ID, false, address
                             , data_buf, getCacheLineSize(), shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_TAGS);
   }
}

void
L2CacheCntlr::processInvFlushCombinedReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   if (shmem_msg->getSingleReceiver() == getTileId())
   {
      shmem_msg->setMsgType(ShmemMsg::FLUSH_REQ);
      processFlushReqFromDramDirectory(sender, shmem_msg);
   }
   else
   {
      shmem_msg->setMsgType(ShmemMsg::INV_REQ);
      processInvReqFromDramDirectory(sender, shmem_msg);
   }
}

void
L2CacheCntlr::updateInvalidationCounters()
{
   if (!_enabled)
      return;

   _total_invalidations ++;
}

void
L2CacheCntlr::updateEvictionCounters(CacheState::Type inserted_cstate, CacheState::Type evicted_cstate)
{
   if (!_enabled)
      return;

   _total_evictions ++;
   switch (inserted_cstate)
   {
   case CacheState::MODIFIED:
      if ((evicted_cstate == CacheState::MODIFIED) || (evicted_cstate == CacheState::OWNED))
         _total_dirty_evictions_exreq ++;
      else if (evicted_cstate == CacheState::SHARED)
         _total_clean_evictions_exreq ++;
      else
         LOG_PRINT_ERROR("Unexpected evicted_cstate(%u)", evicted_cstate);
      break;

   case CacheState::SHARED:
      if ((evicted_cstate == CacheState::MODIFIED) || (evicted_cstate == CacheState::OWNED))
         _total_dirty_evictions_shreq ++;
      else if (evicted_cstate == CacheState::SHARED)
         _total_clean_evictions_shreq ++;
      else
         LOG_PRINT_ERROR("Unexpected evicted_cstate(%u)", evicted_cstate);
      break;

   default:
      LOG_PRINT_ERROR("Unexpected inserted_cstate(%u)", inserted_cstate);
      break;
   }
}

void
L2CacheCntlr::outputSummary(ostream& out)
{
   out << "    L2 Cache Cntlr: " << endl;
   
   out << "      Total Invalidations: " << _total_invalidations << endl;
   out << "      Total Evictions: " << _total_evictions << endl;
   out << "        Exclusive Request - Dirty Evictions: " << _total_dirty_evictions_exreq << endl;
   out << "        Exclusive Request - Clean Evictions: " << _total_clean_evictions_exreq << endl;
   out << "        Shared Request - Dirty Evictions: " << _total_dirty_evictions_shreq << endl;
   out << "        Shared Request - Clean Evictions: " << _total_clean_evictions_shreq << endl;
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

   Cache::MissType cache_miss_type = _L2_cache->updateMissCounters(address, mem_op_type, !cache_hit);
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
      getShmemPerfModel()->incrCurrTime(_L2_cache->getSynchronizationDelay(module));
   }
}

}
