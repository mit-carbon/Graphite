#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
                           L1CacheCntlr* l1_cache_cntlr,
                           AddressHomeLookup* dram_directory_home_lookup,
                           UInt32 cache_line_size,
                           UInt32 l2_cache_size,
                           UInt32 l2_cache_associativity,
                           string l2_cache_replacement_policy,
                           UInt32 l2_cache_access_delay,
                           bool l2_cache_track_miss_types,
                           float frequency)
   : _memory_manager(memory_manager)
   , _l1_cache_cntlr(l1_cache_cntlr)
   , _dram_directory_home_lookup(dram_directory_home_lookup)
{
   _l2_cache_replacement_policy_obj = 
      CacheReplacementPolicy::create(l2_cache_replacement_policy, l2_cache_size, l2_cache_associativity, cache_line_size);
   _l2_cache_hash_fn_obj = new L2CacheHashFn(l2_cache_size, l2_cache_associativity, cache_line_size);
   
   _l2_cache = new Cache("L2",
         HYBRID_PROTOCOL__PP_MOSI__SS,
         Cache::UNIFIED_CACHE,
         L2,
         Cache::WRITE_BACK,
         l2_cache_size,
         l2_cache_associativity,
         cache_line_size,
         _l2_cache_replacement_policy_obj,
         _l2_cache_hash_fn_obj,
         l2_cache_access_delay,
         frequency,
         l2_cache_track_miss_types);
}

L2CacheCntlr::~L2CacheCntlr()
{
   delete _l2_cache;
   delete _l2_cache_replacement_policy_obj;
   delete _l2_cache_hash_fn_obj;
}

void
L2CacheCntlr::readCacheLine(IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length)
{
   data_length = (data_length == UINT32_MAX_) ? getCacheLineSize() : data_length;
   _l2_cache->accessCacheLine(address + offset, Cache::LOAD, data_buf, data_length);
}

void
L2CacheCntlr::writeCacheLine(IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length)
{
   data_length = (data_length == UINT32_MAX_) ? getCacheLineSize() : data_length;
   _l2_cache->accessCacheLine(address + offset, Cache::STORE, data_buf, data_length);
}

void
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf, MemComponent::Type mem_component)
{
   // Construct meta-data info about l2 cache line
   HybridL2CacheLineInfo l2_cache_line_info(_l2_cache->getTag(address), cstate, mem_component);

   // Evicted Line Information
   bool eviction;
   IntPtr evicted_address;
   HybridL2CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   _l2_cache->insertCacheLine(address, &l2_cache_line_info, fill_buf,
                              &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      LOG_PRINT("Eviction: address(%#lx)", evicted_address);
      invalidateCacheLineInL1(evicted_cache_line_info.getCachedLoc(), evicted_address);

      UInt32 dram_directory_home = _dram_directory_home_lookup->getHome(evicted_address);
      CacheState::Type evicted_cstate = evicted_cache_line_info.getCState();

      bool msg_modeled = ::MemoryManager::isMissTypeModeled(Cache::CAPACITY_MISS);

      if ((evicted_cstate == CacheState::MODIFIED) || (evicted_cstate == CacheState::OWNED))
      {
         // Send back the data also
         ShmemMsg eviction_reply(ShmemMsg::FLUSH_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 evicted_address,
                                 writeback_buf, getCacheLineSize(),
                                 getTileId(), msg_modeled);
         getMemoryManager()->sendMsg(dram_directory_home, eviction_reply);
      }
      else
      {
         LOG_ASSERT_ERROR(evicted_cache_line_info.getCState() == CacheState::SHARED,
               "evicted_address(%#lx), evicted_cstate(%u), cached_loc(%u)",
               evicted_address, evicted_cache_line_info.getCState(), evicted_cache_line_info.getCachedLoc());
         
         ShmemMsg eviction_reply(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 evicted_address,
                                 getTileId(), msg_modeled);
         getMemoryManager()->sendMsg(dram_directory_home, eviction_reply);
      }
   }
}

void
L2CacheCntlr::lockCacheLine(IntPtr address)
{
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   l2_cache_line_info.lock();
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

void
L2CacheCntlr::unlockCacheLine(IntPtr address)
{
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   l2_cache_line_info.unlock();
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
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
   if (mem_component != MemComponent::INVALID)
      _l1_cache_cntlr->invalidateCacheLine(mem_component, address);
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
      HybridL2CacheLineInfo evicted_cache_line_info;
      _l2_cache->getCacheLineInfo(evicted_address, &evicted_cache_line_info);
      // Clear the present bit and store the info back
      evicted_cache_line_info.clearCachedLoc(mem_component);
      _l2_cache->setCacheLineInfo(evicted_address, &evicted_cache_line_info);
   }
}

void
L2CacheCntlr::lockCacheLineInL1(MemComponent::Type mem_component, IntPtr address)
{
   if (mem_component != MemComponent::INVALID)
      _l1_cache_cntlr->lockCacheLine(mem_component, address);
}

void
L2CacheCntlr::unlockCacheLineInL1(MemComponent::Type mem_component, IntPtr address)
{
   if (mem_component != MemComponent::INVALID)
      _l1_cache_cntlr->unlockCacheLine(mem_component, address);
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

void
L2CacheCntlr::processMemOpFromL1Cache(MemComponent::Type mem_component,
                                      Core::lock_signal_t lock_signal, 
                                      Core::mem_op_t mem_op_type, 
                                      IntPtr address, UInt32 offset,
                                      Byte* data_buf, UInt32 data_length,
                                      bool modeled)
{
   // Acquire Lock
   acquireLock();

   // Get cache line info
   HybridL2CacheLineInfo l2_cache_line_info;
   pair<bool,Cache::MissType> l2_cache_status = getMemOpStatusInL2Cache(mem_component, address, mem_op_type, l2_cache_line_info);
   bool l2_cache_hit = !l2_cache_status.first;
   
   if (l2_cache_hit)
   {
      Byte cache_line_buf[getCacheLineSize()];
     
      // Write the cache-line in case of a WRITE request 
      if (mem_op_type == Core::WRITE)
         writeCacheLine(address, data_buf, offset, data_length);

      // Read the cache line from L2 cache
      readCacheLine(address, cache_line_buf);

      // Read into the core buffer if it is a READ request
      if ( (mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX) )
         memcpy(data_buf, cache_line_buf + offset, data_length);

      // Get the L2 cache state
      CacheState::Type l2_cstate = l2_cache_line_info.getCState();
      // Insert the cache line in the L1 cache
      insertCacheLineInL1(mem_component, address, l2_cstate, data_buf);
      
      // Set that the cache line in present in the L1 cache in the L2 tags
      l2_cache_line_info.setCachedLoc(mem_component);
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

      // Release Lock
      releaseLock();
      _l1_cache_cntlr->releaseLock(mem_component);
   }
   else
   {
      // Send out a request to the network thread for the cache data
      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());
      // Send the message to the directory
      ShmemMsg shmem_msg(getShmemMsgType(mem_op_type), mem_component, MemComponent::L2_CACHE,
                         address,
                         getTileId(), msg_modeled);
      getMemoryManager()->sendMsg(getTileId(), shmem_msg);

      releaseLock();
      _l1_cache_cntlr->releaseLock(mem_component);

      // Wait till the data is fetched
      _memory_manager->waitForSimThread();

   }
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   acquireLock();

   LOG_ASSERT_ERROR(_outstanding_shmem_msg.getAddress() == INVALID_ADDRESS, 
                    "_outstanding_address(%#lx)", _outstanding_shmem_msg.getAddress());

   // Set Outstanding shmem req parameters
   _outstanding_shmem_msg = *shmem_msg;
   _outstanding_shmem_msg_time = getShmemPerfModel()->getCycleCount();

   ShmemMsg send_shmem_msg(shmem_msg_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                           address,
                           getTileId(), shmem_msg->isModeled()); 
   getMemoryManager()->sendMsg(getDramDirectoryHome(address), send_shmem_msg);

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
   case ShmemMsg::EX_REPLY:
   case ShmemMsg::ASYNC_EX_REPLY:
      processExReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::SH_REPLY:
   case ShmemMsg::ASYNC_SH_REPLY:
      processShReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::UPGRADE_REPLY:
   case ShmemMsg::ASYNC_UPGRADE_REPLY:
      processUpgradeReplyFromDramDirectory(sender, shmem_msg);
      break;

   // Messages to L2 cache from directory to process another request
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
   
   // Shmem Messages due to the REMOTE_ACCESS protocols
   case ShmemMsg::READ_REPLY:
   case ShmemMsg::READ_LOCK_REPLY:
      processReadReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::WRITE_REPLY:
   case ShmemMsg::WRITE_UNLOCK_REPLY:
      processWriteReplyFromDramDirectory(sender, shmem_msg);
      break;

   case ShmemMsg::REMOTE_READ_REQ:
   case ShmemMsg::REMOTE_READ_LOCK_REQ:
      processRemoteReadReqFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::REMOTE_WRITE_REQ:
   case ShmemMsg::REMOTE_WRITE_UNLOCK_REQ:
      processRemoteWriteReqFromDramDirectory(sender, shmem_msg);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized msg type: %u", shmem_msg_type);
      break;
   }

   // Release Locks
   releaseLock();
   if (caching_mem_component != MemComponent::INVALID)
      _l1_cache_cntlr->releaseLock(caching_mem_component);

   if ( (shmem_msg_type == ShmemMsg::EX_REPLY)      || (shmem_msg_type == ShmemMsg::SH_REPLY)    || 
        (shmem_msg_type == ShmemMsg::UPGRADE_REPLY) ||
        (shmem_msg_type == ShmemMsg::READ_REPLY)    || (shmem_msg_type == ShmemMsg::WRITE_REPLY) )
   {
      assert(_outstanding_shmem_msg_time <= getShmemPerfModel()->getCycleCount());
      
      // Reset the clock to the time the request left the tile is miss type is not modeled
      assert(_outstanding_shmem_msg.isModeled() == shmem_msg->isModeled());
      if (!_outstanding_shmem_msg.isModeled())
         getShmemPerfModel()->setCycleCount(_outstanding_shmem_msg_time);

      // Increment the clock by the time taken to update the L2 cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

      // Set the clock of the APP thread to that of the SIM thread
      getShmemPerfModel()->setCycleCount(ShmemPerfModel::_APP_THREAD,
                                         getShmemPerfModel()->getCycleCount());

      // There are no more outstanding memory requests
      _outstanding_shmem_msg = ShmemMsg();
   
      // Wake up the APP thread to signal that data is available   
      _memory_manager->wakeUpAppThread();
   }
}

void
L2CacheCntlr::processReadReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   // Get reply data
   Byte* data_buf = directory_reply->getDataBuf();

   // Check that the request is sane
   ShmemMsg::Type type = _outstanding_shmem_msg.getType();
   assert((type == ShmemMsg::UNIFIED_READ_REQ) || (type == ShmemMsg::UNIFIED_READ_LOCK_REQ));
   
   // Get core_buf and data_length
   Byte* core_buf = _outstanding_shmem_msg.getDataBuf();
   UInt32 data_length = _outstanding_shmem_msg.getDataLength();

   memcpy(core_buf, data_buf, data_length);
}

void
L2CacheCntlr::processWriteReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   // Nothing to do here other than wake up the app thread and return
}

void
L2CacheCntlr::processRemoteReadReqFromDramDirectory(tile_id_t sender, ShmemMsg* remote_req)
{
   IntPtr address = remote_req->getAddress();
   ShmemMsg::Type l2_req_type = remote_req->getType();
   tile_id_t requester = remote_req->getRequester();
   bool modeled = remote_req->isModeled();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
   
   if (l2_cache_line_info.isLocked())
   {
      // Release lock and wait for app thread - this will be quick so waiting is fine
      releaseLock();
      _l1_cache_cntlr->releaseLock(mem_component);
      _memory_manager->waitForAppThread();
   }
   
   // Read the cache line
   UInt32 offset = remote_req->getOffset();
   UInt32 data_length = remote_req->getDataLength();
   Byte data_buf[data_length];
   readCacheLine(address, data_buf, offset, data_length);

   ShmemMsg::Type reply_type;

   if (l2_req_type == ShmemMsg::REMOTE_READ_REQ)
   {
      reply_type = ShmemMsg::REMOTE_READ_REPLY;
   }
   else // (l2_req_type == ShmemMsg::REMOTE_READ_LOCK_REQ)
   {
      reply_type = ShmemMsg::REMOTE_READ_LOCK_REPLY;
      
      // Lock the cache lines in L1-I/L1-D and L2 if it is a read lock request
      MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
      lockCacheLineInL1(mem_component, address);
      l2_cache_line_info.lock();
   }

   // Write-back the cache line info
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   
   // Send REMOTE_READ_REPLY to the directory
   ShmemMsg remote_reply(reply_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                         address,
                         offset, data_length,
                         data_buf, data_length,
                         requester, modeled);
   getMemoryManager()->sendMsg(sender, remote_reply);
}

void
L2CacheCntlr::processRemoteWriteReqFromDramDirectory(tile_id_t sender, ShmemMsg* remote_req)
{
   IntPtr address = remote_req->getAddress();
   ShmemMsg::Type l2_req_type = remote_req->getType();
   tile_id_t requester = remote_req->getRequester();
   bool modeled = remote_req->isModeled();
   
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
   
   if (l2_cache_line_info.isLocked())
   {
      // Release lock and wait for app thread - this will be quick so waiting is fine
      releaseLock();
      _l1_cache_cntlr->releaseLock(mem_component);
      _memory_manager->waitForAppThread();
   }

   ShmemMsg::Type reply_type;

   if (l2_req_type == ShmemMsg::REMOTE_WRITE_REQ)
   {
      reply_type = ShmemMsg::REMOTE_WRITE_REPLY;
      assert(!l2_cache_line_info.isLocked());
   }
   else // (l2_req_type == ShmemMsg::REMOTE_WRITE_UNLOCK_REQ)
   {
      reply_type = ShmemMsg::REMOTE_WRITE_UNLOCK_REPLY;

      assert(l2_cache_line_info.isLocked());

      // Unlock the cache lines in L1-I/L1-D and L2 if it is a write unlock request
      MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
      unlockCacheLineInL1(mem_component, address);
      l2_cache_line_info.unlock();
   }

   // Write-back the cache line info
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   
   // Write the cache line
   UInt32 offset = remote_req->getOffset();
   UInt32 data_length = remote_req->getDataLength();
   Byte* data_buf = remote_req->getDataBuf();
   writeCacheLine(address, data_buf, offset, data_length);
   
   // Send REMOTE_WRITE_REPLY to the directory
   ShmemMsg remote_reply(reply_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                         address,
                         offset, data_length,
                         requester, modeled);
   getMemoryManager()->sendMsg(sender, remote_reply);
}

void
L2CacheCntlr::processExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();

   // Insert Cache Line in L1 and L2 Caches
   insertCacheLineInHierarchy(address, CacheState::MODIFIED, fill_buf);

   // Place data in the core buffer
   ShmemMsg::Type type = _outstanding_shmem_msg.getType();
   assert(type != ShmemMsg::UNIFIED_READ_REQ);
   if (type == ShmemMsg::UNIFIED_READ_LOCK_REQ)
   {
      Byte* core_buf = _outstanding_shmem_msg.getDataBuf();
      UInt32 offset = _outstanding_shmem_msg.getOffset();
      UInt32 data_length = _outstanding_shmem_msg.getDataLength();
      memcpy(core_buf, fill_buf + offset, data_length);
   }
}

void
L2CacheCntlr::processShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();

   // Insert Cache Line in L1 and L2 Caches
   insertCacheLineInHierarchy(address, CacheState::SHARED, fill_buf);
   
   // Place data in the core buffer
   ShmemMsg::Type type = _outstanding_shmem_msg.getType();
   assert(type == ShmemMsg::UNIFIED_READ_REQ);
      
   Byte* core_buf = _outstanding_shmem_msg.getDataBuf();
   UInt32 offset = _outstanding_shmem_msg.getOffset();
   UInt32 data_length = _outstanding_shmem_msg.getDataLength();
   memcpy(core_buf, fill_buf + offset, data_length);
}

void
L2CacheCntlr::processUpgradeReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();

   // Just change state from (SHARED,OWNED) -> MODIFIED
   // In L2
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);

   // Get cache line state
   CacheState::Type l2_cstate = l2_cache_line_info.getCState();
   LOG_ASSERT_ERROR((l2_cstate == CacheState::SHARED) || (l2_cstate == CacheState::OWNED), 
                    "Address(%#lx), State(%u)", address, l2_cstate);

   l2_cache_line_info.setCState(CacheState::MODIFIED);

   // In L1
   LOG_ASSERT_ERROR(address == _outstanding_shmem_msg.getAddress(), 
                    "Got Address(%#lx), Expected Address(%#lx) from Directory",
                    address, _outstanding_shmem_msg.getAddress());

   MemComponent::Type mem_component = _outstanding_shmem_msg.getSenderMemComponent();
   assert(mem_component == MemComponent::L1_DCACHE);
   
   if (l2_cache_line_info.getCachedLoc() == MemComponent::INVALID)
   {
      Byte data_buf[getCacheLineSize()];
      readCacheLine(address, data_buf);

      // Insert cache line in L1
      insertCacheLineInL1(mem_component, address, CacheState::MODIFIED, data_buf);
      // Set cached loc
      l2_cache_line_info.setCachedLoc(mem_component);
   }
   else // (l2_cache_line_info.getCachedLoc() != MemComponent::INVALID)
   {
      setCacheLineStateInL1(mem_component, address, CacheState::MODIFIED);
   }
   
   // Set the meta-date in the L2 cache   
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   
   // Place data in the core buffer
   ShmemMsg::Type type = _outstanding_shmem_msg.getType();
   assert(type != ShmemMsg::UNIFIED_READ_REQ);
   if (type == ShmemMsg::UNIFIED_READ_LOCK_REQ)
   {
      Byte* core_buf = _outstanding_shmem_msg.getDataBuf();
      UInt32 offset = _outstanding_shmem_msg.getOffset();
      UInt32 data_length = _outstanding_shmem_msg.getDataLength();
      
      // Read data from L2 cache
      Byte fill_buf[data_length];
      readCacheLine(address, fill_buf, offset, data_length);
      
      memcpy(core_buf, fill_buf, data_length);
   }
}

void
L2CacheCntlr::processInvReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      assert(cstate == CacheState::SHARED);

      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // SHARED -> INVALID 

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(l2_cache_line_info.getCachedLoc(), address);
      // Invalidate the line in the L2 cache also
      l2_cache_line_info.invalidate();

      // Write-back the cache line info into the L2 cache
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                              address,
                              INVALID_TILE_ID, shmem_msg->isReplyExpected(),
                              shmem_msg->getRequester(), shmem_msg->isModeled());
      getMemoryManager()->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 address,
                                 INVALID_TILE_ID, true,
                                 shmem_msg->getRequester(), shmem_msg->isModeled());
         getMemoryManager()->sendMsg(sender, send_shmem_msg);
      }
   }
}

void
L2CacheCntlr::processFlushReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // (MODIFIED, OWNED, SHARED) -> INVALID
      
      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(l2_cache_line_info.getCachedLoc(), address);

      // Flush the line
      Byte writeback_buf[getCacheLineSize()];
      // First get the cache line to write it back
      readCacheLine(address, writeback_buf);
      // Invalidate the cache line in the L2 cache
      l2_cache_line_info.invalidate();
      
      // Write-back the cache line info into the L2 cache
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::FLUSH_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                              address,
                              writeback_buf, getCacheLineSize(),
                              INVALID_TILE_ID, shmem_msg->isReplyExpected(),
                              shmem_msg->getRequester(), shmem_msg->isModeled());
      getMemoryManager()->sendMsg(sender, send_shmem_msg);
   }
   else
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);

      if (shmem_msg->isReplyExpected())
      {
         ShmemMsg send_shmem_msg(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 address,
                                 INVALID_TILE_ID, true,
                                 shmem_msg->getRequester(), shmem_msg->isModeled());
         getMemoryManager()->sendMsg(sender, send_shmem_msg);
      }
   }
}

void
L2CacheCntlr::processWbReqFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   assert(!shmem_msg->isReplyExpected());

   IntPtr address = shmem_msg->getAddress();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   CacheState::Type cstate = l2_cache_line_info.getCState();

   if (cstate != CacheState::INVALID)
   {
      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // MODIFIED -> OWNED, OWNED -> OWNED, SHARED -> SHARED
      CacheState::Type new_cstate = (cstate == CacheState::MODIFIED) ? CacheState::OWNED : cstate;
      
      // Set the Appropriate Cache State in the L1 cache
      setCacheLineStateInL1(l2_cache_line_info.getCachedLoc(), address, new_cstate);

      // Write-Back the line
      Byte data_buf[getCacheLineSize()];
      // Read the cache line into a local buffer
      readCacheLine(address, data_buf);
      // set the state to OWNED/SHARED
      l2_cache_line_info.setCState(new_cstate);

      // Write-back the new state in the L2 cache
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

      ShmemMsg send_shmem_msg(ShmemMsg::WB_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                              address,
                              data_buf, getCacheLineSize(),
                              shmem_msg->getRequester(), shmem_msg->isModeled());
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
      shmem_msg->setType(ShmemMsg::FLUSH_REQ);
      processFlushReqFromDramDirectory(sender, shmem_msg);
   }
   else
   {
      shmem_msg->setType(ShmemMsg::INV_REQ);
      processInvReqFromDramDirectory(sender, shmem_msg);
   }
}

pair<bool,Cache::MissType>
L2CacheCntlr::getMemOpStatusInL2Cache(MemComponent::Type mem_component, IntPtr address,
                                      Core::mem_op_t mem_op_type, HybridL2CacheLineInfo& l2_cache_line_info)
{
   while (1)
   {
      _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
      bool locked = l2_cache_line_info.isLocked();
      if (!locked)
         break;

      // Release lock and wait for sim thread
      releaseLock();
      _l1_cache_cntlr->releaseLock(mem_component);
      _memory_manager->waitForSimThread();
      // Acquire lock and wake up sim thread
      _l1_cache_cntlr->acquireLock(mem_component);
      acquireLock();
      _memory_manager->wakeUpSimThread();
   }
     
   // Cache line is not locked
   CacheState::Type cstate = l2_cache_line_info.getCState(); 
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

   Cache::MissType cache_miss_type = _l2_cache->updateMissCounters(address, mem_op_type, !cache_hit);

   return make_pair(!cache_hit, cache_miss_type);
}

ShmemMsg::Type
L2CacheCntlr::getShmemMsgType(Core::mem_op_t mem_op_type)
{
   switch(mem_op_type)
   {
   case Core::READ:
      return ShmemMsg::UNIFIED_READ_REQ;
   case Core::READ_EX:
      return ShmemMsg::UNIFIED_READ_LOCK_REQ;
   case Core::WRITE:
      return ShmemMsg::UNIFIED_WRITE_REQ;
   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type(%u)", mem_op_type);
      return ShmemMsg::INVALID;
   }
}

MemComponent::Type
L2CacheCntlr::acquireL1CacheLock(ShmemMsg::Type msg_type, IntPtr address)
{
   switch (msg_type)
   {
   case ShmemMsg::EX_REPLY:
   case ShmemMsg::SH_REPLY:
   case ShmemMsg::UPGRADE_REPLY:
      
      assert(_outstanding_shmem_msg.getAddress() == address);
      assert(_outstanding_shmem_msg.getSenderMemComponent() != MemComponent::INVALID);
      _l1_cache_cntlr->acquireLock(_outstanding_shmem_msg.getSenderMemComponent());
      return _outstanding_shmem_msg.getSenderMemComponent();

   case ShmemMsg::INV_REQ:
   case ShmemMsg::FLUSH_REQ:
   case ShmemMsg::WB_REQ:
   case ShmemMsg::INV_FLUSH_COMBINED_REQ:
   case ShmemMsg::REMOTE_READ_REQ:
   case ShmemMsg::REMOTE_READ_LOCK_REQ:
   case ShmemMsg::REMOTE_WRITE_REQ:
   case ShmemMsg::REMOTE_WRITE_UNLOCK_REQ:
   
      {
         acquireLock();
         
         HybridL2CacheLineInfo l2_cache_line_info;
         _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
         MemComponent::Type caching_mem_component = l2_cache_line_info.getCachedLoc();
         
         releaseLock();

         assert(caching_mem_component != MemComponent::L1_ICACHE);
         if (caching_mem_component != MemComponent::INVALID)
         {
            _l1_cache_cntlr->acquireLock(caching_mem_component);
         }
         return caching_mem_component;
      }

   case ShmemMsg::READ_REPLY:
   case ShmemMsg::READ_LOCK_REPLY:
   case ShmemMsg::WRITE_REPLY:
   case ShmemMsg::WRITE_UNLOCK_REPLY:
      return MemComponent::INVALID;

   default:
      LOG_PRINT_ERROR("Unrecognized Msg Type (%u)", msg_type);
      return MemComponent::INVALID;
   }
}

void
L2CacheCntlr::acquireLock()
{
   _l2_cache_lock.acquire();
}

void
L2CacheCntlr::releaseLock()
{
   _l2_cache_lock.release();
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
