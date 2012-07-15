#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "simulator.h"
#include "config.hpp"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
                           L1CacheCntlr* L1_cache_cntlr,
                           AddressHomeLookup* dram_directory_home_lookup,
                           Semaphore* app_thread_sem,
                           Semaphore* sim_thread_sem,
                           UInt32 cache_line_size,
                           UInt32 L2_cache_size,
                           UInt32 L2_cache_associativity,
                           string L2_cache_replacement_policy,
                           UInt32 L2_cache_access_delay,
                           bool L2_cache_track_miss_types,
                           float frequency)
   : _memory_manager(memory_manager)
   , _L1_cache_cntlr(L1_cache_cntlr)
   , _dram_directory_home_lookup(dram_directory_home_lookup)
   , _app_thread_sem(app_thread_sem)
   , _sim_thread_sem(sim_thread_sem)
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
         _L2_cache_replacement_policy_obj,
         _L2_cache_hash_fn_obj,
         L2_cache_access_delay,
         frequency,
         L2_cache_track_miss_types);

   initializeEvictionCounters();
   initializeInvalidationCounters();

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   initializeUtilizationCounters();
   initializeLifetimeCounters();
#endif
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

#ifdef TRACK_DETAILED_CACHE_COUNTERS
void
L2CacheCntlr::initializeUtilizationCounters()
{
   for (UInt32 i = 0; i < NUM_CACHE_OPERATION_TYPES; i++)
   {
      for (UInt32 j = 0; j <= MAX_TRACKED_UTILIZATION; j++)
      {
         _total_cache_operations_by_utilization[i][j] = 0;
         _modified_state_count_by_utilization[i][j] = 0;
         _shared_state_count_by_utilization[i][j] = 0;
      }
   }
}

void
L2CacheCntlr::initializeLifetimeCounters()
{}
#endif

void
L2CacheCntlr::invalidateCacheLine(IntPtr address, PrL2CacheLineInfo& L2_cache_line_info
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                                  , CacheLineUtilization& net_cache_line_utilization, UInt64 curr_time
#endif
                                 )
{
   L2_cache_line_info.invalidate();
#ifdef TRACK_DETAILED_CACHE_COUNTERS
   L2_cache_line_info.setUtilization(net_cache_line_utilization);
   L2_cache_line_info.setBirthTime(curr_time);
#endif
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
L2CacheCntlr::assertCacheLineWritable(IntPtr address)
{
   PrL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
   CacheState::Type L2_cstate = L2_cache_line_info.getCState();
   assert(CacheState(L2_cstate).writable());
}

void
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component)
{
   UInt64 curr_time = getTime();

   // Construct meta-data info about L2 cache line
   PrL2CacheLineInfo L2_cache_line_info(_L2_cache->getTag(address), cstate, mem_component, curr_time);

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
     
      CacheState::Type evicted_cstate = evicted_cache_line_info.getCState();
     
#ifdef TRACK_DETAILED_CACHE_COUNTERS 
      // Initialize cache line utilization statistics
      AggregateCacheLineUtilization aggregate_utilization;
      // Initialize cache line lifetime
      AggregateCacheLineLifetime aggregate_lifetime; 

      // Get cache line utilization in L1-I/L1-D
      _L1_cache_cntlr->updateAggregateCacheLineUtilization(aggregate_utilization, evicted_cache_line_info.getCachedLoc(), evicted_address);
      // Get cache line lifetime in L1-I/L1-D
      _L1_cache_cntlr->updateAggregateCacheLineLifetime(aggregate_lifetime, evicted_cache_line_info.getCachedLoc(), evicted_address, curr_time);
     
      // Get cache line utilization in L2 + previous L1-I/L1-D
      aggregate_utilization += evicted_cache_line_info.getAggregateUtilization();
      // Get cache line lifetime in L2 + previous L1-I/L1-D
      aggregate_lifetime += evicted_cache_line_info.getAggregateLifetime(curr_time);
    
      // Net cache line utilization
      CacheLineUtilization net_cache_line_utilization;
      net_cache_line_utilization.read = aggregate_utilization.L1_I.read + aggregate_utilization.L1_D.read + aggregate_utilization.L2.read;
      net_cache_line_utilization.write = aggregate_utilization.L1_I.write + aggregate_utilization.L1_D.write + aggregate_utilization.L2.write;
      
      // Update counters that track the utilization of the cache line
      UInt64 total_utilization = aggregate_utilization.total(); 
      updateUtilizationCounters(CACHE_EVICTION, aggregate_utilization, evicted_cstate);

      // Update counters that track the lifetime of the cache line
      updateLifetimeCounters(CACHE_EVICTION, aggregate_lifetime, total_utilization);
#endif

      // Invalidate the cache line in L1-I/L1-D + get utilization
      invalidateCacheLineInL1(evicted_cache_line_info.getCachedLoc(), evicted_address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , net_cache_line_utilization, curr_time
#endif
                             );

      UInt32 home_node_id = getHome(evicted_address);

      bool msg_modeled = ::MemoryManager::isMissTypeModeled(Cache::CAPACITY_MISS);

      // Update eviction counters so as to track clean and dirty evictions
      updateEvictionCounters(cstate, evicted_cstate);

      if ((evicted_cstate == CacheState::MODIFIED) || (evicted_cstate == CacheState::OWNED))
      {
         // Send back the data also
         ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                                , getTileId(), INVALID_TILE_ID, false, evicted_address
                                , writeback_buf, getCacheLineSize(), msg_modeled
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                                , total_utilization, aggregate_lifetime.L2
#endif
                                );
         getMemoryManager()->sendMsg(home_node_id, send_shmem_msg);
      }
      else
      {
         LOG_ASSERT_ERROR(evicted_cache_line_info.getCState() == CacheState::SHARED,
                          "evicted_address(%#lx), evicted_cstate(%u), cached_loc(%u)",
                          evicted_address, evicted_cache_line_info.getCState(), evicted_cache_line_info.getCachedLoc());
         
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                                , getTileId(), INVALID_TILE_ID, false, evicted_address
                                ,  msg_modeled
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                                ,  total_utilization, aggregate_lifetime.L2
#endif
                                );
         getMemoryManager()->sendMsg(home_node_id, send_shmem_msg);
      }
   }
   else // no eviction - update lifetime
   {
      assert(!evicted_cache_line_info.isValid());

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      CacheLineUtilization cache_line_utilization = evicted_cache_line_info.getUtilization();
      // We need to also include the effect of cache lines that were invalidated before the private copy threshold
      UInt64 transient_lifetime = evicted_cache_line_info.getLifetime(curr_time);

      // Update the global lifetime counters
      updateLifetimeCounters(CACHE_INVALIDATION, MemComponent::L2_CACHE, transient_lifetime, cache_line_utilization.total());
#endif
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
L2CacheCntlr::invalidateCacheLineInL1(MemComponent::Type mem_component, IntPtr address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                                     , CacheLineUtilization& net_cache_line_utilization, UInt64 curr_time
#endif
                                     )
{
   if (mem_component != MemComponent::INVALID)
   {
      _L1_cache_cntlr->invalidateCacheLine(mem_component, address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                                           , net_cache_line_utilization, curr_time
#endif
                                           );
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

   // Get curr time
   UInt64 curr_time = getTime();

   // Insert the Cache Line in L1 Cache
   _L1_cache_cntlr->insertCacheLine(mem_component, address, cstate, fill_buf,
                                    &eviction, &evicted_cache_line_info, &evicted_address,
                                    curr_time);

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   // Get the utilization of the cache line
   CacheLineUtilization cache_line_utilization = evicted_cache_line_info.getUtilization();
#endif
  
   if (eviction)
   {
      // Evicted cache line is valid
      assert(evicted_cache_line_info.isValid());

      // Clear the Present bit in L2 Cache corresponding to the evicted line
      // Get the cache line info first
      PrL2CacheLineInfo L2_cache_line_info;
      _L2_cache->getCacheLineInfo(evicted_address, &L2_cache_line_info);
      
      // Clear the present bit and store the info back
      L2_cache_line_info.clearCachedLoc(mem_component);
    
#ifdef TRACK_DETAILED_CACHE_COUNTERS 
      // Update utilization counters in the L2 cache
      L2_cache_line_info.incrUtilization(mem_component, cache_line_utilization);
   
      // Compute the lifetime of the cache line
      UInt64 lifetime = evicted_cache_line_info.getLifetime(curr_time);
      // Update lifetime counters in the L2 cache
      L2_cache_line_info.incrLifetime(mem_component, lifetime);
#endif

      // Update the cache with the new line info
      _L2_cache->setCacheLineInfo(evicted_address, &L2_cache_line_info);
   }
   else // no eviction
   {
      // Evicted cache line is NOT valid
      assert(!evicted_cache_line_info.isValid());
   
#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Compute the lifetime of the cache line
      UInt64 lifetime = evicted_cache_line_info.getLifetime(curr_time);
      updateLifetimeCounters(CACHE_INVALIDATION, mem_component, lifetime, cache_line_utilization.total());
#endif
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
      L2_cache_line_info.setCachedLoc(mem_component);
      _L2_cache->setCacheLineInfo(address, &L2_cache_line_info);
   }
   
   return shmem_request_status_in_L2_cache;
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   acquireLock();

   assert(shmem_msg->getDataBuf() == NULL);
   assert(shmem_msg->getDataLength() == 0);

   LOG_ASSERT_ERROR(_outstanding_shmem_msg.getAddress() == INVALID_ADDRESS, 
                    "_outstanding_address(%#llx)", _outstanding_shmem_msg.getAddress());

   // Set Outstanding shmem req parameters
   _outstanding_shmem_msg = *shmem_msg;
   _outstanding_shmem_msg_time = getShmemPerfModel()->getCycleCount();

   ShmemMsg send_shmem_msg(shmem_msg_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                           getTileId(), INVALID_TILE_ID, false, address, shmem_msg->isModeled()); 
   getMemoryManager()->sendMsg(getHome(address), send_shmem_msg);

   releaseLock();
}

void
L2CacheCntlr::handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   IntPtr address = shmem_msg->getAddress();

   // Acquire Locks
   MemComponent::Type caching_mem_component = acquireL1CacheLock(shmem_msg_type, address);
   acquireLock();

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

   // Release Locks
   releaseLock();
   if (caching_mem_component != MemComponent::INVALID)
      _L1_cache_cntlr->releaseLock(caching_mem_component);

   if ((shmem_msg_type == ShmemMsg::EX_REP) || (shmem_msg_type == ShmemMsg::SH_REP) || (shmem_msg_type == ShmemMsg::UPGRADE_REP))
   {
      assert(_outstanding_shmem_msg_time <= getShmemPerfModel()->getCycleCount());
      
      // Reset the clock to the time the request left the tile is miss type is not modeled
      LOG_ASSERT_ERROR(_outstanding_shmem_msg.isModeled() == shmem_msg->isModeled(), "Request(%s), Response(%s)",
                       _outstanding_shmem_msg.isModeled() ? "MODELED" : "UNMODELED",
                       shmem_msg->isModeled() ? "MODELED" : "UNMODELED");

      if (!_outstanding_shmem_msg.isModeled())
         getShmemPerfModel()->setCycleCount(_outstanding_shmem_msg_time);

      // Increment the clock by the time taken to update the L2 cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

      // Set the clock of the APP thread to that of the SIM thread
      getShmemPerfModel()->setCycleCount(ShmemPerfModel::_APP_THREAD,
                                         getShmemPerfModel()->getCycleCount());

      // There are no more outstanding memory requests
      _outstanding_shmem_msg = ShmemMsg();
      
      wakeUpAppThread();
      waitForAppThread();
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
   CacheState::Type L2_cstate = L2_cache_line_info.getCState();
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
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // SHARED -> INVALID 

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Get the time
      UInt64 curr_time = getTime();

      // Aggregate Cache Line Utilization
      AggregateCacheLineUtilization aggregate_utilization;
      // Aggregate Cache Line Lifetime
      AggregateCacheLineLifetime aggregate_lifetime;

      // Get the utilization counters from L2, previous L1-I, L1-D
      aggregate_utilization += L2_cache_line_info.getAggregateUtilization();
      // Get the lifetime counters from L2, previous L1-I, L1-D
      aggregate_lifetime += L2_cache_line_info.getAggregateLifetime(curr_time);

      // Get the utilization counters from L1-I, L1-D
      _L1_cache_cntlr->updateAggregateCacheLineUtilization(aggregate_utilization, L2_cache_line_info.getCachedLoc(), address);
      // Get the lifetime counters from L1-I, L1-D
      _L1_cache_cntlr->updateAggregateCacheLineLifetime(aggregate_lifetime, L2_cache_line_info.getCachedLoc(), address, curr_time);

      // Net cache line utilization
      CacheLineUtilization net_cache_line_utilization;
      net_cache_line_utilization.read = aggregate_utilization.L1_I.read + aggregate_utilization.L1_D.read + aggregate_utilization.L2.read;
      net_cache_line_utilization.write = aggregate_utilization.L1_I.write + aggregate_utilization.L1_D.write + aggregate_utilization.L2.write;

      // Update utilization counters on invalidation
      UInt64 total_utilization = net_cache_line_utilization.total();
      updateUtilizationCounters(CACHE_INVALIDATION, aggregate_utilization, cstate);
      
      // Update lifetime counters
      updateLifetimeCounters(CACHE_INVALIDATION, aggregate_lifetime, total_utilization);
#endif

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(L2_cache_line_info.getCachedLoc(), address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , net_cache_line_utilization, curr_time
#endif
                             );
      
      // Invalidate the line in the L2 cache
      updateInvalidationCounters();
      invalidateCacheLine(address, L2_cache_line_info
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                         , net_cache_line_utilization, curr_time
#endif
                         );

      ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                             , shmem_msg->getRequester(), INVALID_TILE_ID, shmem_msg->isReplyExpected(), address
                             , shmem_msg->isModeled()
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , total_utilization, aggregate_lifetime.L2
#endif
                             );
      getMemoryManager()->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 shmem_msg->getRequester(), INVALID_TILE_ID, true, address,
                                 shmem_msg->isModeled());
         getMemoryManager()->sendMsg(sender, send_shmem_msg);
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
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // (MODIFIED, OWNED, SHARED) -> INVALID
     
#ifdef TRACK_DETAILED_CACHE_COUNTERS 
      // Get the time in ns
      UInt64 curr_time = getTime();
      
      // Initialize the aggregate cache line utilization counters
      AggregateCacheLineUtilization aggregate_utilization;
      // Initialize the aggregate cache line lifetime counters
      AggregateCacheLineLifetime aggregate_lifetime;

      // Get the utilization counters from L2, previous L1-I, L1-D
      aggregate_utilization += L2_cache_line_info.getAggregateUtilization();
      // Get the lifetime counters from L2, previous L1-I, L1-D
      aggregate_lifetime += L2_cache_line_info.getAggregateLifetime(curr_time);

      // Get the utilization counters from L1-I, L1-D
      _L1_cache_cntlr->updateAggregateCacheLineUtilization(aggregate_utilization, L2_cache_line_info.getCachedLoc(), address);
      // Get the lifetime counters from L1-I, L1-D
      _L1_cache_cntlr->updateAggregateCacheLineLifetime(aggregate_lifetime, L2_cache_line_info.getCachedLoc(), address, curr_time);

      // Net cache line utilization
      CacheLineUtilization net_cache_line_utilization;
      net_cache_line_utilization.read = aggregate_utilization.L1_I.read + aggregate_utilization.L1_D.read + aggregate_utilization.L2.read;
      net_cache_line_utilization.write = aggregate_utilization.L1_I.write + aggregate_utilization.L1_D.write + aggregate_utilization.L2.write;

      // Update utilization counters on invalidation
      UInt64 total_utilization = aggregate_utilization.total();
      updateUtilizationCounters(CACHE_INVALIDATION, aggregate_utilization, cstate);

      // Update lifetime counters on invalidation
      updateLifetimeCounters(CACHE_INVALIDATION, aggregate_lifetime, total_utilization);
#endif

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(L2_cache_line_info.getCachedLoc(), address
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , net_cache_line_utilization, curr_time
#endif
                             );

      // Flush the line
      Byte data_buf[getCacheLineSize()];
      // First get the cache line to write it back
      readCacheLine(address, data_buf);
   
      // Invalidate the cache line in the L2 cache
      updateInvalidationCounters();
      invalidateCacheLine(address, L2_cache_line_info
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                         , net_cache_line_utilization, curr_time
#endif
                         );

      ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY
                             , shmem_msg->getRequester(), INVALID_TILE_ID, shmem_msg->isReplyExpected(), address
                             , data_buf, getCacheLineSize(), shmem_msg->isModeled()
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , total_utilization, aggregate_lifetime.L2
#endif
                             );
      getMemoryManager()->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 shmem_msg->getRequester(), INVALID_TILE_ID, true, address,
                                 shmem_msg->isModeled());
         getMemoryManager()->sendMsg(sender, send_shmem_msg);
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
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(L2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

#ifdef TRACK_DETAILED_CACHE_COUNTERS
      // Get the curr time in ns
      UInt64 curr_time = getTime();

      // Initialize cache line utilization counters
      AggregateCacheLineUtilization aggregate_utilization;
      // Initialize cache line lifetime counters
      AggregateCacheLineLifetime aggregate_lifetime;

      // Read the utilization counters from L1-I, L1-D, L2
      aggregate_utilization += L2_cache_line_info.getAggregateUtilization();
      // Read the lifetime from L1-I, L1-D, L2
      aggregate_lifetime += L2_cache_line_info.getAggregateLifetime(curr_time);

      // Get the utilization counters from L1-I/L1-D
      _L1_cache_cntlr->updateAggregateCacheLineUtilization(aggregate_utilization, L2_cache_line_info.getCachedLoc(), address);
      // Get the lifetime counters from L1-I/L1-D
      _L1_cache_cntlr->updateAggregateCacheLineLifetime(aggregate_lifetime, L2_cache_line_info.getCachedLoc(), address, curr_time);
      
      UInt64 total_utilization = aggregate_utilization.total();
#endif

      // MODIFIED -> OWNED, OWNED -> OWNED, SHARED -> SHARED
      CacheState::Type new_cstate = (cstate == CacheState::MODIFIED) ? CacheState::OWNED : cstate;
      
      // Set the Appropriate Cache State in the L1 cache
      setCacheLineStateInL1(L2_cache_line_info.getCachedLoc(), address, new_cstate);

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
                             , data_buf, getCacheLineSize(), shmem_msg->isModeled()
#ifdef TRACK_DETAILED_CACHE_COUNTERS
                             , total_utilization, aggregate_lifetime.L2
#endif
                             );
      getMemoryManager()->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);
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

#ifdef TRACK_DETAILED_CACHE_COUNTERS
void
L2CacheCntlr::updateUtilizationCounters(CacheOperationType operation, AggregateCacheLineUtilization& aggregate_utilization, CacheState::Type cstate)
{
   if (!_enabled)
      return;

   UInt64 line_utilization = aggregate_utilization.total();
   if (line_utilization > MAX_TRACKED_UTILIZATION)
   {
      aggregate_utilization = (aggregate_utilization * MAX_TRACKED_UTILIZATION) / line_utilization;
      line_utilization = MAX_TRACKED_UTILIZATION;
      // LOG_ASSERT_WARNING(line_utilization == aggregate_utilization.total(), "Eviction: line_utilization(%lu), total(%lu)",
      //                    line_utilization, aggregate_utilization.total());
   }
   _utilization_counters[operation][line_utilization] += aggregate_utilization;
   _total_cache_operations_by_utilization[operation][line_utilization] ++;

   if ((cstate == CacheState::MODIFIED) || (cstate == CacheState::OWNED))
      _modified_state_count_by_utilization[operation][line_utilization] ++;
   else // SHARED
      _shared_state_count_by_utilization[operation][line_utilization] ++;
}

void
L2CacheCntlr::updateLifetimeCounters(CacheOperationType operation, AggregateCacheLineLifetime& aggregate_lifetime, UInt64 cache_line_utilization)
{
   if (!_enabled)
      return;

   if (cache_line_utilization > MAX_TRACKED_UTILIZATION)
      cache_line_utilization = MAX_TRACKED_UTILIZATION;

   _lifetime_counters[operation][cache_line_utilization] += aggregate_lifetime;
}

void
L2CacheCntlr::updateLifetimeCounters(CacheOperationType operation, MemComponent::Type mem_component, UInt64 cache_line_lifetime, UInt64 cache_line_utilization)
{
   if (!_enabled)
      return;

   // Initialize cache line lifetime counters
   AggregateCacheLineLifetime aggregate_lifetime;

   switch (mem_component)
   {
   case MemComponent::L1_ICACHE:
      aggregate_lifetime.L1_I = cache_line_lifetime;
      break;
   case MemComponent::L1_DCACHE:
      aggregate_lifetime.L1_D = cache_line_lifetime;
      break;
   case MemComponent::L2_CACHE:
      aggregate_lifetime.L2 = cache_line_lifetime;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized mem component(%u)", mem_component);
      break;
   }
      
   updateLifetimeCounters(operation, aggregate_lifetime, cache_line_utilization);
}
#endif

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

#ifdef TRACK_DETAILED_CACHE_COUNTERS
   outputUtilizationCountSummary(out);
   outputLifetimeCountSummary(out);
#endif
}
   
#ifdef TRACK_DETAILED_CACHE_COUNTERS
void
L2CacheCntlr::outputUtilizationCountSummary(ostream& out)
{
   out << "      Utilization Summary (Eviction): " << endl;
   for (UInt32 i = 0; i <= MAX_TRACKED_UTILIZATION; i++)
   {
      out << "        Utilization-"       << i << ": " << _total_cache_operations_by_utilization[CACHE_EVICTION][i] << endl;
      out << "          Modified State: " << _modified_state_count_by_utilization[CACHE_EVICTION][i] << endl;
      out << "          Shared State: "   << _shared_state_count_by_utilization[CACHE_EVICTION][i] << endl;
      out << "          L1-I read: "      << _utilization_counters[CACHE_EVICTION][i].L1_I.read << endl;
      out << "          L1-D read: "      << _utilization_counters[CACHE_EVICTION][i].L1_D.read << endl;
      out << "          L1-D write: "     << _utilization_counters[CACHE_EVICTION][i].L1_D.write << endl;
      out << "          L2 read: "        << _utilization_counters[CACHE_EVICTION][i].L2.read << endl;
      out << "          L2 write: "       << _utilization_counters[CACHE_EVICTION][i].L2.write << endl;
   }

   out << "      Utilization Summary (Invalidation): " << endl;
   for (UInt32 i = 0; i <= MAX_TRACKED_UTILIZATION; i++)
   {
      out << "        Utilization-"       << i << ": " << _total_cache_operations_by_utilization[CACHE_INVALIDATION][i] << endl;
      out << "          Modified State: " << _modified_state_count_by_utilization[CACHE_INVALIDATION][i] << endl;
      out << "          Shared State: "   << _shared_state_count_by_utilization[CACHE_INVALIDATION][i] << endl;
      out << "          L1-I read: "      << _utilization_counters[CACHE_INVALIDATION][i].L1_I.read << endl;
      out << "          L1-D read: "      << _utilization_counters[CACHE_INVALIDATION][i].L1_D.read << endl;
      out << "          L1-D write: "     << _utilization_counters[CACHE_INVALIDATION][i].L1_D.write << endl;
      out << "          L2 read: "        << _utilization_counters[CACHE_INVALIDATION][i].L2.read << endl;
      out << "          L2 write: "       << _utilization_counters[CACHE_INVALIDATION][i].L2.write << endl;
   }
}

void
L2CacheCntlr::outputLifetimeCountSummary(ostream& out)
{
   config::Config* cfg = Sim()->getCfg();
   UInt32 total_cache_lines_L1_I = cfg->getInt("l1_icache/T1/cache_size") * 1024 / cfg->getInt("l1_icache/T1/cache_line_size");
   UInt32 total_cache_lines_L1_D = cfg->getInt("l1_dcache/T1/cache_size") * 1024 / cfg->getInt("l1_dcache/T1/cache_line_size");
   UInt32 total_cache_lines_L2 = cfg->getInt("l2_cache/T1/cache_size") * 1024 / cfg->getInt("l2_cache/T1/cache_line_size");

   out << "      Lifetime Summary (Eviction): " << endl;
   for (UInt32 i = 0; i <= MAX_TRACKED_UTILIZATION; i++)
   {
      out << "        Utilization-"    << i << ": " << endl;
      out << "          L1-I Lifetime: " << ((float) _lifetime_counters[CACHE_EVICTION][i].L1_I) / total_cache_lines_L1_I << endl;
      out << "          L1-D Lifetime: " << ((float) _lifetime_counters[CACHE_EVICTION][i].L1_D) / total_cache_lines_L1_D << endl;
      out << "          L2 Lifetime: "   << ((float) _lifetime_counters[CACHE_EVICTION][i].L2) / total_cache_lines_L2 << endl;
   }

   out << "      Lifetime Summary (Invalidation): " << endl;
   for (UInt32 i = 0; i <= MAX_TRACKED_UTILIZATION; i++)
   {
      out << "        Utilization-"    << i << ": " << endl;
      out << "          L1-I Lifetime: " << ((float) _lifetime_counters[CACHE_INVALIDATION][i].L1_I) / total_cache_lines_L1_I << endl;
      out << "          L1-D Lifetime: " << ((float) _lifetime_counters[CACHE_INVALIDATION][i].L1_D) / total_cache_lines_L1_D << endl;
      out << "          L2 Lifetime: "   << ((float) _lifetime_counters[CACHE_INVALIDATION][i].L2) / total_cache_lines_L2 << endl;
   }
}
#endif

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

MemComponent::Type
L2CacheCntlr::acquireL1CacheLock(ShmemMsg::Type shmem_msg_type, IntPtr address)
{
   switch (shmem_msg_type)
   {
   case ShmemMsg::EX_REP:
   case ShmemMsg::SH_REP:
   case ShmemMsg::UPGRADE_REP:
      
      assert(_outstanding_shmem_msg.getAddress() == address);
      assert(_outstanding_shmem_msg.getSenderMemComponent() != MemComponent::INVALID);
      _L1_cache_cntlr->acquireLock(_outstanding_shmem_msg.getSenderMemComponent());
      return _outstanding_shmem_msg.getSenderMemComponent();

   case ShmemMsg::INV_REQ:
   case ShmemMsg::FLUSH_REQ:
   case ShmemMsg::WB_REQ:
   case ShmemMsg::INV_FLUSH_COMBINED_REQ:
   
      {
         acquireLock();
         
         PrL2CacheLineInfo L2_cache_line_info;
         _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);
         MemComponent::Type caching_mem_component = L2_cache_line_info.getCachedLoc();
         
         releaseLock();

         if (caching_mem_component != MemComponent::INVALID)
         {
            _L1_cache_cntlr->acquireLock(caching_mem_component);
         }
         return caching_mem_component;
      }

   default:
      LOG_PRINT_ERROR("Unrecognized Msg Type (%u)", shmem_msg_type);
      return MemComponent::INVALID;
   }
}

UInt64
L2CacheCntlr::getTime()
{
   volatile float frequency = getMemoryManager()->getTile()->getCore()->getPerformanceModel()->getFrequency();
   assert(frequency == 1.0);
   return (UInt64) (((double) getShmemPerfModel()->getCycleCount()) / frequency);
}

void
L2CacheCntlr::acquireLock()
{
   _L2_cache_lock.acquire();
}

void
L2CacheCntlr::releaseLock()
{
   _L2_cache_lock.release();
}

void
L2CacheCntlr::wakeUpAppThread()
{
   _app_thread_sem->signal();
}

void
L2CacheCntlr::waitForAppThread()
{
   _sim_thread_sem->wait();
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

}
