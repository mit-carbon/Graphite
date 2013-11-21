#include "l1_cache_cntlr.h"
#include "memory_manager.h"
#include "config.h"
#include "log.h"

namespace PrL1ShL2MSI
{

L1CacheCntlr::L1CacheCntlr(MemoryManager* memory_manager,
                           AddressHomeLookup* L2_cache_home_lookup,
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
   , _L2_cache_home_lookup(L2_cache_home_lookup)
{
   _L1_icache_replacement_policy_obj = 
      CacheReplacementPolicy::create(L1_icache_replacement_policy, L1_icache_size, L1_icache_associativity, cache_line_size);
   _L1_dcache_replacement_policy_obj = 
      CacheReplacementPolicy::create(L1_dcache_replacement_policy, L1_dcache_size, L1_dcache_associativity, cache_line_size);
   _L1_icache_hash_fn_obj = new CacheHashFn(L1_icache_size, L1_icache_associativity, cache_line_size);
   _L1_dcache_hash_fn_obj = new CacheHashFn(L1_dcache_size, L1_dcache_associativity, cache_line_size);

   _L1_icache = new Cache("L1-I",
         PR_L1_SH_L2_MSI,
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
         L1_icache_track_miss_types);
   _L1_dcache = new Cache("L1-D",
         PR_L1_SH_L2_MSI,
         Cache::DATA_CACHE,
         L1,
         Cache::WRITE_BACK,
         L1_dcache_size,
         L1_dcache_associativity, 
         cache_line_size,
         L1_dcache_num_banks,
         _L1_dcache_replacement_policy_obj,
         _L1_dcache_hash_fn_obj,
         L1_dcache_data_access_cycles,
         L1_dcache_tags_access_cycles,
         L1_dcache_perf_model_type,
         L1_dcache_track_miss_types);
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
      LOG_ASSERT_ERROR((access_num == 1) || (access_num == 2), "access_num(%u)", access_num);

      // Wake up the sim thread after acquiring the lock
      if (access_num == 2)
      {
         _memory_manager->wakeUpSimThread();
      }

      pair<bool, Cache::MissType> cache_miss_info = operationPermissibleinL1Cache(mem_component, ca_address, mem_op_type, access_num);
      bool cache_hit = !cache_miss_info.first;
      if (cache_hit)
      {
         // Increment Shared Mem Perf model current time
         // L1 Cache
         _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_DATA_AND_TAGS);

         accessCache(mem_component, mem_op_type, ca_address, offset, data_buf, data_length);
                 
         return L1_cache_hit;
      }

      LOG_ASSERT_ERROR(access_num == 1, "Should find line in cache on second access");
      // Expect to find address in the L1-I/L1-D cache if there is an UNLOCK signal
      LOG_ASSERT_ERROR(lock_signal != Core::UNLOCK, "Expected to find address(%#lx) in L1 Cache", ca_address);

      _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_TAGS);

      // The memory request misses in the L1 cache
      L1_cache_hit = false;

      // Send out a request to the network thread for the cache data
      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());
      ShmemMsg::Type shmem_msg_type = getShmemMsgType(mem_op_type);
      ShmemMsg shmem_msg(shmem_msg_type, MemComponent::CORE, mem_component,
                         getTileId(), false, ca_address,
                         msg_modeled);
      _memory_manager->sendMsg(getTileId(), shmem_msg);

      // Wait for the sim thread
      _memory_manager->waitForSimThread();
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
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type: %u", mem_op_type);
      break;
   }
}

pair<bool, Cache::MissType>
L1CacheCntlr::operationPermissibleinL1Cache(MemComponent::Type mem_component,
                                            IntPtr address, Core::mem_op_t mem_op_type,
                                            UInt32 access_num)
{
   PrL1CacheLineInfo L1_cache_line_info;
   getCacheLineInfo(mem_component, address, &L1_cache_line_info);
   CacheState::Type cstate = L1_cache_line_info.getCState();
   
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
      LOG_PRINT_ERROR("Unsupported mem_op_type: %u", mem_op_type);
      break;
   }

   Cache::MissType cache_miss_type = Cache::INVALID_MISS_TYPE;
   if (access_num == 1)
   {
      // Update the Cache Counters
      cache_miss_type = getL1Cache(mem_component)->updateMissCounters(address, mem_op_type, !cache_hit);
   }

   return make_pair(!cache_hit, cache_miss_type);
}

void
L1CacheCntlr::getCacheLineInfo(MemComponent::Type mem_component, IntPtr address, PrL1CacheLineInfo* L1_cache_line_info)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);
   L1_cache->getCacheLineInfo(address, L1_cache_line_info);
}

void
L1CacheCntlr::setCacheLineInfo(MemComponent::Type mem_component, IntPtr address, PrL1CacheLineInfo* L1_cache_line_info)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);
   L1_cache->setCacheLineInfo(address, L1_cache_line_info);
}

void
L1CacheCntlr::readCacheLine(MemComponent::Type mem_component, IntPtr address, Byte* data_buf)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);
   L1_cache->accessCacheLine(address, Cache::LOAD, data_buf, getCacheLineSize());
}

void
L1CacheCntlr::insertCacheLine(MemComponent::Type mem_component, IntPtr address, CacheState::Type cstate, Byte* fill_buf)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   PrL1CacheLineInfo L1_cache_line_info(L1_cache->getTag(address), cstate);

   bool eviction;
   IntPtr evicted_address;
   PrL1CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   L1_cache->insertCacheLine(address, &L1_cache_line_info, fill_buf,
                             &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      assert(evicted_cache_line_info.isValid());
      LOG_PRINT("evicted address(%#lx)", evicted_address);

      UInt32 L2_cache_home = getL2CacheHome(evicted_address);
      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());

      CacheState::Type evicted_cstate = evicted_cache_line_info.getCState();
      if (evicted_cstate == CacheState::MODIFIED)
      {
         // Send back the data also
         ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, mem_component, MemComponent::L2_CACHE,
                                 getTileId(), false, evicted_address,
                                 writeback_buf, getCacheLineSize(),
                                 msg_modeled);
         _memory_manager->sendMsg(L2_cache_home, send_shmem_msg);
      }
      else
      {
         LOG_ASSERT_ERROR(evicted_cstate == CacheState::SHARED, "evicted_address(%#lx), evicted_cstate(%u)",
                          evicted_address, evicted_cstate);
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, mem_component, MemComponent::L2_CACHE,
                                 getTileId(), false, evicted_address,
                                 msg_modeled);
         _memory_manager->sendMsg(L2_cache_home, send_shmem_msg);
      }
   }
}

void
L1CacheCntlr::invalidateCacheLine(MemComponent::Type mem_component, IntPtr address)
{
   Cache* L1_cache = getL1Cache(mem_component);
   assert(L1_cache);

   // Invalidate cache line
   PrL1CacheLineInfo L1_cache_line_info;
   L1_cache->getCacheLineInfo(address, &L1_cache_line_info);
   L1_cache_line_info.invalidate();
   L1_cache->setCacheLineInfo(address, &L1_cache_line_info);
}

void
L1CacheCntlr::handleMsgFromCore(ShmemMsg* shmem_msg)
{
   _outstanding_shmem_msg = *shmem_msg;
   _outstanding_shmem_msg_time = getShmemPerfModel()->getCurrTime();

   IntPtr address = shmem_msg->getAddress();
   // Send msg out to L2 cache
   ShmemMsg send_shmem_msg(shmem_msg->getType(), shmem_msg->getReceiverMemComponent(), MemComponent::L2_CACHE,
                           shmem_msg->getRequester(), false, address,
                           shmem_msg->isModeled());
   tile_id_t receiver = _L2_cache_home_lookup->getHome(address);
   _memory_manager->sendMsg(receiver, send_shmem_msg);
}

void
L1CacheCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // L2 Cache synchronization delay 
   if (sender == getTileId())
      getShmemPerfModel()->incrCurrTime(getL1Cache(shmem_msg->getReceiverMemComponent())->getSynchronizationDelay(L2_CACHE));
   else{
      getShmemPerfModel()->incrCurrTime(getL1Cache(shmem_msg->getReceiverMemComponent())->getSynchronizationDelay(NETWORK_MEMORY));
   }

   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   switch (shmem_msg_type)
   {
   case ShmemMsg::EX_REP:
      processExRepFromL2Cache(sender, shmem_msg);
      break;
   case ShmemMsg::SH_REP:
      processShRepFromL2Cache(sender, shmem_msg);
      break;
   case ShmemMsg::UPGRADE_REP:
      processUpgradeRepFromL2Cache(sender, shmem_msg);
      break;
   case ShmemMsg::INV_REQ:
      processInvReqFromL2Cache(sender, shmem_msg);
      break;
   case ShmemMsg::FLUSH_REQ:
      processFlushReqFromL2Cache(sender, shmem_msg);
      break;
   case ShmemMsg::WB_REQ:
      processWbReqFromL2Cache(sender, shmem_msg);
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
                       _outstanding_shmem_msg.isModeled() ? "MODELED" : "UNMODELED", shmem_msg->isModeled() ? "MODELED" : "UNMODELED");

      // If not modeled, set the time back to the original time message was sent out
      if (!_outstanding_shmem_msg.isModeled())
         getShmemPerfModel()->setCurrTime(_outstanding_shmem_msg_time);

      // Increment the clock by the time taken to update the L1-I/L1-D cache
      _memory_manager->incrCurrTime(shmem_msg->getReceiverMemComponent(), CachePerfModel::ACCESS_DATA_AND_TAGS);

      // There are no more outstanding memory requests
      _outstanding_shmem_msg = ShmemMsg();
     
      // Wake up the app thread and wait for it to complete one memory operation 
      _memory_manager->wakeUpAppThread();
      _memory_manager->waitForAppThread();
   }
}

void
L1CacheCntlr::processExRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   Byte* data_buf = shmem_msg->getDataBuf();
   MemComponent::Type mem_component = shmem_msg->getReceiverMemComponent();

   assert(address == _outstanding_shmem_msg.getAddress());
   // Insert Cache Line in L1-I/L1-D Cache
   insertCacheLine(mem_component, address, CacheState::MODIFIED, data_buf);
}

void
L1CacheCntlr::processShRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   Byte* data_buf = shmem_msg->getDataBuf();
   MemComponent::Type mem_component = shmem_msg->getReceiverMemComponent();

   assert(address == _outstanding_shmem_msg.getAddress());
   // Insert Cache Line in L1-I/L1-D Cache
   insertCacheLine(mem_component, address, CacheState::SHARED, data_buf);
}

void
L1CacheCntlr::processUpgradeRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   LOG_ASSERT_ERROR(shmem_msg->getReceiverMemComponent() == MemComponent::L1_DCACHE,
                    "Unexpected mem component(%u)", shmem_msg->getReceiverMemComponent());

   assert(address == _outstanding_shmem_msg.getAddress());
   
   // Just change state from SHARED -> MODIFIED
   PrL1CacheLineInfo L1_cache_line_info;
   getCacheLineInfo(MemComponent::L1_DCACHE, address, &L1_cache_line_info);

   // Get cache line state
   __attribute__((unused)) CacheState::Type L1_cstate = L1_cache_line_info.getCState();
   LOG_ASSERT_ERROR(L1_cstate == CacheState::SHARED, "Address(%#lx), State(%u)", address, L1_cstate);

   L1_cache_line_info.setCState(CacheState::MODIFIED);

   // Set the meta-data in the L1-I/L1-D cache   
   setCacheLineInfo(MemComponent::L1_DCACHE, address, &L1_cache_line_info);
}

void
L1CacheCntlr::processInvReqFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   MemComponent::Type mem_component = shmem_msg->getReceiverMemComponent();

   PrL1CacheLineInfo L1_cache_line_info;
   getCacheLineInfo(mem_component, address, &L1_cache_line_info);
   CacheState::Type cstate = L1_cache_line_info.getCState();

   // Update Shared Mem perf counters for access to L1-D Cache
   _memory_manager->incrCurrTime(mem_component, CachePerfModel::ACCESS_TAGS);

   if (cstate != CacheState::INVALID)
   {
      LOG_ASSERT_ERROR(cstate == CacheState::SHARED, "cstate(%u)", cstate);

      // SHARED -> INVALID 

      // Invalidate the line in L1-D Cache
      invalidateCacheLine(mem_component, address);
      
      ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, mem_component, MemComponent::L2_CACHE,
                              shmem_msg->getRequester(), shmem_msg->isReplyExpected(), address,
                              shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REP, mem_component, MemComponent::L2_CACHE,
                                 shmem_msg->getRequester(), true, address,
                                 shmem_msg->isModeled());
         _memory_manager->sendMsg(sender, send_shmem_msg);
      }
   }
}

void
L1CacheCntlr::processFlushReqFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   assert(!shmem_msg->isReplyExpected());
   IntPtr address = shmem_msg->getAddress();
   LOG_ASSERT_ERROR(shmem_msg->getReceiverMemComponent() == MemComponent::L1_DCACHE,
                    "Unexpected mem component(%u)", shmem_msg->getReceiverMemComponent());

   PrL1CacheLineInfo L1_cache_line_info;
   getCacheLineInfo(MemComponent::L1_DCACHE, address, &L1_cache_line_info);
   CacheState::Type cstate = L1_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // MODIFIED -> INVALID
      LOG_ASSERT_ERROR(cstate == CacheState::MODIFIED, "cstate(%u)", cstate);
      
      // Update Shared Mem perf counters for access to L1 Cache
      _memory_manager->incrCurrTime(MemComponent::L1_DCACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

      // Flush the line
      Byte data_buf[getCacheLineSize()];
      // First get the cache line to write it back
      readCacheLine(MemComponent::L1_DCACHE, address, data_buf);
   
      // Invalidate the cache line in the L1-I/L1-D cache
      invalidateCacheLine(MemComponent::L1_DCACHE, address);

      ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REP, MemComponent::L1_DCACHE, MemComponent::L2_CACHE,
                              shmem_msg->getRequester(), false, address,
                              data_buf, getCacheLineSize(),
                              shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L1-D cache tags
      _memory_manager->incrCurrTime(MemComponent::L1_DCACHE, CachePerfModel::ACCESS_TAGS);
   }
}

void
L1CacheCntlr::processWbReqFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   assert(!shmem_msg->isReplyExpected());

   IntPtr address = shmem_msg->getAddress();
   LOG_ASSERT_ERROR(shmem_msg->getReceiverMemComponent() == MemComponent::L1_DCACHE,
                    "Unexpected mem component(%u)", shmem_msg->getReceiverMemComponent());
   // No requests can come externally to the L1-I cache since it contains
   // only instructions and no self modifying code can be handled
   // Also, no reading from a remote cache is allowed

   PrL1CacheLineInfo L1_cache_line_info;
   getCacheLineInfo(MemComponent::L1_DCACHE, address, &L1_cache_line_info);
   CacheState::Type cstate = L1_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // MODIFIED -> SHARED
      LOG_ASSERT_ERROR(cstate == CacheState::MODIFIED, "cstate(%u)", cstate);

      // Update shared memory performance counters for access to L1 Cache
      _memory_manager->incrCurrTime(MemComponent::L1_DCACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

      // Write-Back the line
      Byte data_buf[getCacheLineSize()];
      // Read the cache line into a local buffer
      readCacheLine(MemComponent::L1_DCACHE, address, data_buf);
      // set the state to SHARED
      L1_cache_line_info.setCState(CacheState::SHARED);

      // Write-back the new state in the L1 cache
      setCacheLineInfo(MemComponent::L1_DCACHE, address, &L1_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::WB_REP, MemComponent::L1_DCACHE, MemComponent::L2_CACHE,
                              shmem_msg->getRequester(), false, address,
                              data_buf, getCacheLineSize(),
                              shmem_msg->isModeled());
      _memory_manager->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to the L1-D Cache
      _memory_manager->incrCurrTime(MemComponent::L1_DCACHE, CachePerfModel::ACCESS_TAGS);
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
      LOG_PRINT_ERROR("Unrecognized Memory Component(%u)", mem_component);
      return NULL;
   }
}

tile_id_t
L1CacheCntlr::getL2CacheHome(IntPtr address)
{
   return _L2_cache_home_lookup->getHome(address);
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
      return ShmemMsg::INVALID_MSG_TYPE;
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
