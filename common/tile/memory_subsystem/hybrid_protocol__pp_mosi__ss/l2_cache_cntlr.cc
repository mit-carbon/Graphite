#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

#define IS_WRITE(x)        (x == Core::WRITE)

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
   _l2_cache_replacement_policy_obj = new L2CacheReplacementPolicy(l2_cache_size, l2_cache_associativity, cache_line_size);
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
L2CacheCntlr::insertCacheLine(IntPtr address, CacheState::Type cstate, bool locked,
                              Byte* fill_buf, MemComponent::Type mem_component)
{
   LOG_PRINT("insertCacheLine[Address(%#lx), CState(%s), Locked(%s), MemComponent(%s)] start",
             address, SPELL_CSTATE(cstate), SPELL_LOCKED(locked), SPELL_MEMCOMP(mem_component));

   // Construct meta-data info about l2 cache line
   HybridL2CacheLineInfo l2_cache_line_info(_l2_cache->getTag(address), cstate, locked, mem_component);

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
      LOG_ASSERT_ERROR(!evicted_cache_line_info.isLocked(), "Address(%#lx) locked while evicted", evicted_address);

      // Get cache line utilization
      UInt32 cache_line_utilization = getLineUtilizationInCacheHierarchy(evicted_address, evicted_cache_line_info);

      // Invalidate the line in L1-I/L1-D cache
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
                                 true,
                                 getTileID(), msg_modeled,
                                 cache_line_utilization);
         getMemoryManager()->sendMsg(dram_directory_home, eviction_reply);
      }
      else
      {
         LOG_ASSERT_ERROR((evicted_cstate == CacheState::SHARED) || (evicted_cstate == CacheState::EXCLUSIVE),
               "evicted_address(%#lx), evicted_cstate(%s), cached_loc(%u)",
               evicted_address, SPELL_CSTATE(evicted_cache_line_info.getCState()), evicted_cache_line_info.getCachedLoc());
         
         ShmemMsg eviction_reply(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                                 evicted_address,
                                 getTileID(), msg_modeled,
                                 cache_line_utilization);
         getMemoryManager()->sendMsg(dram_directory_home, eviction_reply);
      }
   }
   
   LOG_PRINT("insertCacheLine[Address(%#lx), CState(%s), Locked(%s), MemComponent(%s)] end",
             address, SPELL_CSTATE(cstate), SPELL_LOCKED(locked), SPELL_MEMCOMP(mem_component));
}

void
L2CacheCntlr::invalidateCacheLine(IntPtr address)
{
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   assert(l2_cache_line_info.isValid());
   l2_cache_line_info.invalidate();
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

CacheState::Type
L2CacheCntlr::getCacheLineState(IntPtr address)
{
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   return l2_cache_line_info.getCState();
}

void
L2CacheCntlr::setCacheLineState(IntPtr address, CacheState::Type cstate)
{
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   l2_cache_line_info.setCState(cstate);
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

void
L2CacheCntlr::lockCacheLine(IntPtr address)
{
   LOG_PRINT("Locking Cache Line (%#lx)", address);
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   assert(!l2_cache_line_info.isLocked());
   l2_cache_line_info.lock();
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
}

void
L2CacheCntlr::unlockCacheLine(IntPtr address)
{
   LOG_PRINT("Unlocking Cache Line (%#lx)", address);
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   assert(l2_cache_line_info.isLocked());
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
   LOG_PRINT("insertCacheLineInL1[Address(%#lx), CState(%s), MemComponent(%s)] start",
             address, SPELL_CSTATE(cstate), SPELL_MEMCOMP(mem_component));

   assert(mem_component != MemComponent::INVALID);

   bool l1_eviction;
   IntPtr l1_evicted_address;
   UInt32 l1_evicted_line_utilization;

   // Insert the Cache Line in L1 Cache
   _l1_cache_cntlr->insertCacheLine(mem_component, address, cstate, fill_buf,
                                    l1_eviction, l1_evicted_address, l1_evicted_line_utilization);

   if (l1_eviction)
   {
      // Clear the Present bit in L2 Cache corresponding to the evicted line
      // Get the cache line info first
      HybridL2CacheLineInfo evicted_cache_line_info;
      _l2_cache->getCacheLineInfo(l1_evicted_address, &evicted_cache_line_info);
      // Add the L1 utilization
      evicted_cache_line_info.incrUtilization(l1_evicted_line_utilization);
      // Clear the present bit and store the info back
      evicted_cache_line_info.clearCachedLoc(mem_component);
      _l2_cache->setCacheLineInfo(l1_evicted_address, &evicted_cache_line_info);
   }
   
   LOG_PRINT("insertCacheLineInL1[Address(%#lx), CState(%s), MemComponent(%s)] end",
             address, SPELL_CSTATE(cstate), SPELL_MEMCOMP(mem_component));
}

void
L2CacheCntlr::insertCacheLineInHierarchy(IntPtr address, CacheState::Type cstate, bool locked, Byte* fill_buf)
{
   LOG_PRINT("insertCacheLineInHierarchy[Address(%#lx), CState(%s), Locked(%s)]",
             address, SPELL_CSTATE(cstate), SPELL_LOCKED(locked));
   LOG_ASSERT_ERROR(address == _outstanding_mem_op.getAddress(), 
                    "Got Address(%#lx), Expected Address(%#lx) from Directory",
                    address, _outstanding_mem_op.getAddress());
   
   MemComponent::Type mem_component = _outstanding_mem_op.getMemComponent();
  
   // Insert Line in the L2 cache
   insertCacheLine(address, cstate, locked, fill_buf, mem_component);
   
   // Insert Line in the L1 cache
   insertCacheLineInL1(mem_component, address, cstate, fill_buf);
}

void
L2CacheCntlr::readDataFromCoreBuffer(ShmemMsg* directory_reply)
{
   Byte* fill_buf = directory_reply->getDataBuf();
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   Core::lock_signal_t lock_signal = _outstanding_mem_op.getLockSignal();
   LOG_ASSERT_ERROR((mem_op_type != Core::WRITE) || (lock_signal != Core::UNLOCK),
                    "mem_op_type(WRITE) && lock_signal(UNLOCK)")
   if ( (mem_op_type == Core::WRITE) && (lock_signal == Core::NONE) )
   {
      Byte* core_buf = _outstanding_mem_op.getDataBuf();
      UInt32 offset = _outstanding_mem_op.getOffset();
      UInt32 data_length = _outstanding_mem_op.getDataLength();
      memcpy(fill_buf + offset, core_buf, data_length);
   }
}

void
L2CacheCntlr::writeDataToCoreBuffer(ShmemMsg* directory_reply)
{
   Byte* fill_buf = directory_reply->getDataBuf();
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   if ( (mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX) )
   {
      Byte* core_buf = _outstanding_mem_op.getDataBuf();
      UInt32 offset = _outstanding_mem_op.getOffset();
      UInt32 data_length = _outstanding_mem_op.getDataLength();
      memcpy(core_buf, fill_buf + offset, data_length);
   }
}

UInt32
L2CacheCntlr::getLineUtilizationInCacheHierarchy(IntPtr address, HybridL2CacheLineInfo& l2_cache_line_info)
{
   UInt32 l2_utilization = l2_cache_line_info.getUtilization();
   MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
   if (mem_component != MemComponent::INVALID)
   {
      UInt32 l1_utilization = _l1_cache_cntlr->getCacheLineUtilization(mem_component, address);
      return l1_utilization + l2_utilization;
   }
   return l2_utilization;
}

void
L2CacheCntlr::assertL2CacheReady(Core::mem_op_t mem_op_type, CacheState::Type cstate)
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
L2CacheCntlr::processMemOpL2CacheReady(MemComponent::Type mem_component,
                                       Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                       IntPtr address, Byte* data_buf, UInt32 offset, UInt32 data_length,
                                       HybridL2CacheLineInfo& l2_cache_line_info)
{
   // Check if L2 cache is indeed ready
   assertL2CacheReady(mem_op_type, l2_cache_line_info.getCState());
   
   // Write the cache-line in case of a WRITE request
   if (mem_op_type == Core::WRITE)
   {
      writeCacheLine(address, data_buf, offset, data_length);
      l2_cache_line_info.setCState(CacheState::MODIFIED);
   }

   // Read the cache line from L2 cache
   Byte cache_line_buf[getCacheLineSize()];
   readCacheLine(address, cache_line_buf);

   // Get the L2 cache state
   CacheState::Type cstate = l2_cache_line_info.getCState();
   bool locked = (mem_op_type == Core::READ_EX);
   if (locked)
      l2_cache_line_info.lock();
  
   MemComponent::Type cached_loc = l2_cache_line_info.getCachedLoc();
   if (cached_loc == MemComponent::INVALID)
   { 
      // Set that the cache line in present in the L1 cache in the L2 tags
      l2_cache_line_info.setCachedLoc(mem_component);

      // Insert the cache line in the L1 cache
      insertCacheLineInL1(mem_component, address, cstate, cache_line_buf);
   }
   else // (cached_loc != MemComponent::INVALID)
   {
      LOG_ASSERT_ERROR(cached_loc == mem_component, "cached_loc(%s), mem_component(%s)",
                       cached_loc, mem_component);
      _l1_cache_cntlr->processMemOpL1CacheReady(mem_component, mem_op_type, lock_signal,
                                                address, data_buf, offset, data_length);
   }
   
   // Store cache line info in the L2 cache   
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   
   // Read into the core buffer if it is a READ/READ_EX request
   if ( (mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX) )
      memcpy(data_buf, cache_line_buf + offset, data_length);
}

bool
L2CacheCntlr::processMemOpFromL1Cache(MemComponent::Type mem_component,
                                      Core::mem_op_t mem_op_type, 
                                      Core::lock_signal_t lock_signal, 
                                      IntPtr address, Byte* data_buf,
                                      UInt32 offset, UInt32 data_length,
                                      bool modeled)
{
   // Get cache line info
   HybridL2CacheLineInfo l2_cache_line_info;
   pair<bool,Cache::MissType> l2_cache_status = getMemOpStatusInL2Cache(mem_component, address,
                                                                        mem_op_type, lock_signal,
                                                                        l2_cache_line_info);
   bool l2_cache_hit = !l2_cache_status.first;
   
   if (l2_cache_hit)
   {
      l2_cache_line_info.incrUtilization();
      processMemOpL2CacheReady(mem_component, mem_op_type, lock_signal,
                               address, data_buf, offset, data_length, l2_cache_line_info);
      return true;
   }
   else
   {
      // Send out a request to the network thread for the cache data
      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileID());

      ShmemMsg::Type directory_req_type = getDirectoryReqType(mem_op_type, lock_signal);

      // Set outstanding msg and time at which msg was sent
      LOG_ASSERT_ERROR(_outstanding_mem_op.getAddress() == INVALID_ADDRESS,
                       "_outstanding_address(%#lx)", _outstanding_mem_op.getAddress());
      _outstanding_mem_op = MemOp(mem_component, mem_op_type, lock_signal,
                                  address, data_buf, offset, data_length,
                                  msg_modeled);
      _outstanding_mem_op_time = getShmemPerfModel()->getCycleCount();

      // Send the message to the directory
      Byte* send_data_buf = IS_WRITE(mem_op_type) ? data_buf : (Byte*) NULL;
      UInt32 send_data_buf_size = IS_WRITE(mem_op_type) ? data_length : 0;
      ShmemMsg shmem_msg(directory_req_type, mem_component, MemComponent::L2_CACHE,
                         address,
                         offset, data_length,
                         send_data_buf, send_data_buf_size,
                         getTileID(), msg_modeled);
      getMemoryManager()->sendMsg(getTileID(), shmem_msg);
   
      return false;
   }
}

void
L2CacheCntlr::handleMsgFromL1Cache(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::Type directory_req_type = shmem_msg->getType();

   ShmemMsg send_shmem_msg(directory_req_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                           address,
                           shmem_msg->getOffset(), shmem_msg->getDataLength(),
                           shmem_msg->getDataBuf(), shmem_msg->getDataBufSize(),
                           shmem_msg->getRequester(), shmem_msg->isModeled());
   getMemoryManager()->sendMsg(getDramDirectoryHome(address), send_shmem_msg);
}

void
L2CacheCntlr::handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   IntPtr address = shmem_msg->getAddress();

   // Acquire Locks
   bool locked = acquireLock(shmem_msg_type, address);

   switch (shmem_msg_type)
   {
   case ShmemMsg::EX_REPLY:
      processExReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::SH_REPLY:
      processShReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::READY_REPLY:
      processReadyReplyFromDramDirectory(sender, shmem_msg);
      break;
   
   case ShmemMsg::ASYNC_EX_REPLY:
      processAsyncExReplyFromDramDirectory(sender, shmem_msg);
      break;
   case ShmemMsg::ASYNC_SH_REPLY:
      processAsyncShReplyFromDramDirectory(sender, shmem_msg);
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
   releaseLock(shmem_msg_type, address, locked);

   if ( (shmem_msg_type == ShmemMsg::EX_REPLY)        || (shmem_msg_type == ShmemMsg::SH_REPLY)             ||
        (shmem_msg_type == ShmemMsg::READY_REPLY)     ||
        (shmem_msg_type == ShmemMsg::READ_REPLY)      || (shmem_msg_type == ShmemMsg::WRITE_REPLY)          ||
        (shmem_msg_type == ShmemMsg::READ_LOCK_REPLY) || (shmem_msg_type == ShmemMsg::WRITE_UNLOCK_REPLY) )
   {
      LOG_ASSERT_ERROR(_outstanding_mem_op_time <= getShmemPerfModel()->getCycleCount(),
                       "Outstanding mem op time(%llu), Curr cycle count(%llu)",
                       _outstanding_mem_op_time, getShmemPerfModel()->getCycleCount());
      
      // Reset the clock to the time the request left the tile is miss type is not modeled
      assert(_outstanding_mem_op.isModeled() == shmem_msg->isModeled());
      if (!_outstanding_mem_op.isModeled())
         getShmemPerfModel()->setCycleCount(_outstanding_mem_op_time);

      // Increment the clock by the time taken to update the L2 cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);

      // Set the clock of the APP thread to that of the SIM thread
      getShmemPerfModel()->setCycleCount(ShmemPerfModel::_APP_THREAD,
                                         getShmemPerfModel()->getCycleCount());

      // There are no more outstanding memory requests
      _outstanding_mem_op = MemOp();
   
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
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   LOG_ASSERT_ERROR((mem_op_type == Core::READ) || (mem_op_type == Core::READ_EX),
                    "mem_op_type(%s), address(%#lx), requester(%i), address(%#lx), reply_type(%s)",
                    SPELL_MEMOP(mem_op_type), _outstanding_mem_op.getAddress(),
                    directory_reply->getRequester(), directory_reply->getAddress(),
                    SPELL_SHMSG(directory_reply->getType()));
   
   // Get core_buf and data_length
   Byte* core_buf = _outstanding_mem_op.getDataBuf();
   UInt32 data_length = _outstanding_mem_op.getDataLength();

   memcpy(core_buf, data_buf, data_length);
}

void
L2CacheCntlr::processWriteReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   // Nothing to do here other than wake up the app thread and return
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   LOG_ASSERT_ERROR(mem_op_type == Core::WRITE, "mem_op_type(%s)", SPELL_MEMOP(mem_op_type));
}

void
L2CacheCntlr::processRemoteReadReqFromDramDirectory(tile_id_t sender, ShmemMsg* remote_req)
{
   IntPtr address = remote_req->getAddress();
   ShmemMsg::Type req_type = remote_req->getType();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   if (!l2_cache_line_info.isValid())
      return;

   MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
   
   // Read the cache line
   UInt32 offset = remote_req->getOffset();
   UInt32 data_length = remote_req->getDataLength();
   Byte data_buf[data_length];
   readCacheLine(address, data_buf, offset, data_length);

   ShmemMsg::Type reply_type;

   if (req_type == ShmemMsg::REMOTE_READ_REQ)
   {
      reply_type = ShmemMsg::REMOTE_READ_REPLY;
   }
   else // (req_type == ShmemMsg::REMOTE_READ_LOCK_REQ)
   {
      reply_type = ShmemMsg::REMOTE_READ_LOCK_REPLY;
      
      // Lock the cache lines in L2 if it is a read lock request
      l2_cache_line_info.lock();
   }

   if (mem_component != MemComponent::INVALID)
   {
      UInt32 l1_utilization = _l1_cache_cntlr->getCacheLineUtilization(mem_component, address);
      l2_cache_line_info.incrUtilization(l1_utilization);
      l2_cache_line_info.clearCachedLoc(mem_component);
   }

   // Write-back the cache line info
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);

   // Invalidate the line in the L1 cache   
   invalidateCacheLineInL1(mem_component, address);
   
   // Send REMOTE_READ_REPLY to the directory
   ShmemMsg remote_reply(reply_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                         address,
                         data_buf, data_length,
                         remote_req->getRequester(), remote_req->isModeled());
   getMemoryManager()->sendMsg(sender, remote_reply);
}

void
L2CacheCntlr::processRemoteWriteReqFromDramDirectory(tile_id_t sender, ShmemMsg* remote_req)
{
   IntPtr address = remote_req->getAddress();
   ShmemMsg::Type req_type = remote_req->getType();
   
   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   if (!l2_cache_line_info.isValid())
      return;
   
   MemComponent::Type mem_component = l2_cache_line_info.getCachedLoc();
   
   ShmemMsg::Type reply_type;

   l2_cache_line_info.setCState(CacheState::MODIFIED);
   if (req_type == ShmemMsg::REMOTE_WRITE_REQ)
   {
      reply_type = ShmemMsg::REMOTE_WRITE_REPLY;
      assert(!l2_cache_line_info.isLocked());
   }
   else // (req_type == ShmemMsg::REMOTE_WRITE_UNLOCK_REQ)
   {
      reply_type = ShmemMsg::REMOTE_WRITE_UNLOCK_REPLY;
      assert(l2_cache_line_info.isLocked());
      // Unlock the cache lines in L2 if it is a write unlock request
      l2_cache_line_info.unlock();
   }

   if (mem_component != MemComponent::INVALID)
   {
      UInt32 l1_utilization = _l1_cache_cntlr->getCacheLineUtilization(mem_component, address);
      l2_cache_line_info.incrUtilization(l1_utilization);
      l2_cache_line_info.clearCachedLoc(mem_component);
   }

   // Write-back the cache line info
   _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
   
   // Write the cache line in L2
   UInt32 offset = remote_req->getOffset();
   UInt32 data_length = remote_req->getDataLength();
   Byte* data_buf = remote_req->getDataBuf();
   writeCacheLine(address, data_buf, offset, data_length);

   // Invalidate the line in the L1 cache   
   invalidateCacheLineInL1(mem_component, address);

   // Send REMOTE_WRITE_REPLY to the directory
   ShmemMsg remote_reply(reply_type, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                         address,
                         remote_req->getRequester(), remote_req->isModeled());
   getMemoryManager()->sendMsg(sender, remote_reply);
}

void
L2CacheCntlr::processExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   // Read data from the core buffer (if it is a WRITE/WRITE_UNLOCK req)
   readDataFromCoreBuffer(directory_reply);

   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();

   LOG_PRINT("processExReplyFromDramDirectory[sender(%i), address(%#lx)]", sender, address);

   // Insert Cache Line in L1 and L2 Caches
   CacheState::Type cstate = ((directory_reply->isCacheLineDirty()) || IS_WRITE(mem_op_type))
                             ? CacheState::MODIFIED : CacheState::EXCLUSIVE;
   bool locked = (mem_op_type == Core::READ_EX);
   insertCacheLineInHierarchy(address, cstate, locked, fill_buf);
   LOG_PRINT("Finished inserting cache line(%#lx) in hierarchy", address);

   // Write data to the core buffer from directory reply (if it is a READ/READ_LOCK)
   writeDataToCoreBuffer(directory_reply);
}

void
L2CacheCntlr::processShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   LOG_ASSERT_ERROR(mem_op_type == Core::READ, "mem_op_type(%s)", SPELL_MEMOP(mem_op_type));

   LOG_PRINT("processShReplyFromDramDirectory[sender(%i), address(%#lx)]", sender, address);

   // Insert Cache Line in L1 and L2 Caches
   CacheState::Type cstate = directory_reply->isCacheLineDirty() ? CacheState::OWNED : CacheState::SHARED;
   insertCacheLineInHierarchy(address, cstate, false, fill_buf);
   LOG_PRINT("Finished inserting cache line(%#lx) in hierarchy", address);
  
   // Write data to the core buffer from directory reply (if it is a READ/READ_LOCK)
   writeDataToCoreBuffer(directory_reply);
}

void
L2CacheCntlr::processReadyReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   MemComponent::Type mem_component = _outstanding_mem_op.getMemComponent();
   Core::mem_op_t mem_op_type = _outstanding_mem_op.getType();
   Core::lock_signal_t lock_signal = _outstanding_mem_op.getLockSignal();
   IntPtr address = _outstanding_mem_op.getAddress();
   Byte* data_buf = _outstanding_mem_op.getDataBuf();
   UInt32 offset = _outstanding_mem_op.getOffset();
   UInt32 data_length = _outstanding_mem_op.getDataLength();

   HybridL2CacheLineInfo l2_cache_line_info;
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);

   // Check whether the L2 cache is ready
   processMemOpL2CacheReady(mem_component, mem_op_type, lock_signal,
                            address, data_buf, offset, data_length, l2_cache_line_info);
}

void
L2CacheCntlr::processAsyncExReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();

   LOG_PRINT("processAsyncExReplyFromDramDirectory[sender(%i), address(%#lx)]", sender, address);

   CacheState::Type cstate = directory_reply->isCacheLineDirty() ? CacheState::MODIFIED : CacheState::EXCLUSIVE;
   insertCacheLine(address, cstate, false, fill_buf, MemComponent::INVALID);
}

void
L2CacheCntlr::processAsyncShReplyFromDramDirectory(tile_id_t sender, ShmemMsg* directory_reply)
{
   IntPtr address = directory_reply->getAddress();
   Byte* fill_buf = directory_reply->getDataBuf();

   LOG_PRINT("processAsyncShReplyFromDramDirectory[sender(%i), address(%#lx)]", sender, address);

   CacheState::Type cstate = directory_reply->isCacheLineDirty() ? CacheState::OWNED : CacheState::SHARED;
   insertCacheLine(address, cstate, false, fill_buf, MemComponent::INVALID);
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
      LOG_ASSERT_ERROR(cstate == CacheState::SHARED, "address(%#lx), cstate(%s)", address, SPELL_CSTATE(cstate));

      // Update Shared Mem perf counters for access to L2 Cache
      getMemoryManager()->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_TAGS);
      // Update Shared Mem perf counters for access to L1 Cache
      getMemoryManager()->incrCycleCount(l2_cache_line_info.getCachedLoc(), CachePerfModel::ACCESS_CACHE_TAGS);

      // Get total cache line utilization
      UInt32 cache_line_utilization = getLineUtilizationInCacheHierarchy(address, l2_cache_line_info);

      // SHARED -> INVALID 

      // Invalidate the line in L1 Cache
      invalidateCacheLineInL1(l2_cache_line_info.getCachedLoc(), address);
      // Invalidate the line in the L2 cache also
      l2_cache_line_info.invalidate();

      // Write-back the cache line info into the L2 cache
      _l2_cache->setCacheLineInfo(address, &l2_cache_line_info);
      
      ShmemMsg send_shmem_msg(ShmemMsg::INV_REPLY, MemComponent::L2_CACHE, MemComponent::DRAM_DIRECTORY,
                              address,
                              shmem_msg->getRequester(), shmem_msg->isModeled(),
                              cache_line_utilization);
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

      // Get total cache line utilization
      UInt32 cache_line_utilization = getLineUtilizationInCacheHierarchy(address, l2_cache_line_info);

      // (MODIFIED, EXCLUSIVE, OWNED, SHARED) -> INVALID
      bool cache_line_dirty = ((cstate == CacheState::MODIFIED) || (cstate == CacheState::OWNED));
      
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
                              cache_line_dirty,
                              shmem_msg->getRequester(), shmem_msg->isModeled(),
                              cache_line_utilization);
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

      // Get total cache line utilization
      UInt32 cache_line_utilization = getLineUtilizationInCacheHierarchy(address, l2_cache_line_info);

      // MODIFIED -> OWNED, EXCLUSIVE -> SHARED, OWNED -> OWNED, SHARED -> SHARED
      bool cache_line_dirty = ((cstate == CacheState::MODIFIED) || (cstate == CacheState::OWNED));
      CacheState::Type new_cstate = cstate;
      if (cstate == CacheState::MODIFIED)
         new_cstate = CacheState::OWNED;
      else if (cstate == CacheState::EXCLUSIVE)
         new_cstate = CacheState::SHARED;
      
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
                              cache_line_dirty,
                              shmem_msg->getRequester(), shmem_msg->isModeled(),
                              cache_line_utilization);
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
   if (shmem_msg->getSingleReceiver() == getTileID())
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
                                      Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal,
                                      HybridL2CacheLineInfo& l2_cache_line_info)
{
   _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
   bool locked = l2_cache_line_info.isValid() && l2_cache_line_info.isLocked();
   assert( (lock_signal != Core::UNLOCK) || (!l2_cache_line_info.isValid()) );

   if (locked)
   {
      _memory_manager->waitOnThreadForCacheLineUnlock(ShmemPerfModel::_SIM_THREAD, address);
      // Re-access the cache once unlocked
      _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
      bool unlocked = l2_cache_line_info.isValid() && !l2_cache_line_info.isLocked();
      LOG_ASSERT_ERROR(unlocked, "Cache line[Valid(%s), MemComponent(%s), Address(%#lx), MemOp(%u), LockSignal(%u)] still locked",
                       l2_cache_line_info.isValid() ? "TRUE" : "FALSE",
                       SPELL_MEMCOMP(mem_component), address, mem_op_type, lock_signal);
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
L2CacheCntlr::getDirectoryReqType(Core::mem_op_t mem_op_type, Core::lock_signal_t lock_signal)
{
   switch(mem_op_type)
   {
   case Core::READ:
      return ShmemMsg::UNIFIED_READ_REQ;
   case Core::READ_EX:
      return ShmemMsg::UNIFIED_READ_LOCK_REQ;
   case Core::WRITE:
      if (lock_signal == Core::NONE)
         return ShmemMsg::UNIFIED_WRITE_REQ;
      else // (lock_signal == Core::LOCK)
         return ShmemMsg::WRITE_UNLOCK_REQ;
   default:
      LOG_PRINT_ERROR("Unsupported Mem Op Type(%u)", mem_op_type);
      return ShmemMsg::INVALID;
   }
}

bool
L2CacheCntlr::acquireLock(ShmemMsg::Type msg_type, IntPtr address)
{
   switch (msg_type)
   {
   case ShmemMsg::EX_REPLY:
   case ShmemMsg::SH_REPLY:
   case ShmemMsg::READY_REPLY:
   case ShmemMsg::ASYNC_EX_REPLY:
   case ShmemMsg::ASYNC_SH_REPLY:
      _memory_manager->acquireLock();
      return true;
   
   case ShmemMsg::INV_REQ:
   case ShmemMsg::FLUSH_REQ:
   case ShmemMsg::WB_REQ:
   case ShmemMsg::INV_FLUSH_COMBINED_REQ:
   
   case ShmemMsg::REMOTE_READ_REQ:
   case ShmemMsg::REMOTE_READ_LOCK_REQ:
   case ShmemMsg::REMOTE_WRITE_REQ:
      
      {
         _memory_manager->acquireLock();
         HybridL2CacheLineInfo l2_cache_line_info;
         _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
         bool locked = (l2_cache_line_info.isValid() && l2_cache_line_info.isLocked());
         if (locked)
         {
            _memory_manager->waitOnThreadForCacheLineUnlock(ShmemPerfModel::_APP_THREAD, address);
            // Check cache line again to see if it is unlocked
            _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
            LOG_ASSERT_ERROR(l2_cache_line_info.isValid() && !l2_cache_line_info.isLocked(),
                             "Cache Line for [Address(%#lx), MsgType(%s)] still locked",
                             address, SPELL_SHMSG(msg_type));
         }
         return true;
      }

   case ShmemMsg::REMOTE_WRITE_UNLOCK_REQ:
      {
         _memory_manager->acquireLock();
         HybridL2CacheLineInfo l2_cache_line_info;
         _l2_cache->getCacheLineInfo(address, &l2_cache_line_info);
         bool locked = (l2_cache_line_info.isValid() && l2_cache_line_info.isLocked());
         LOG_ASSERT_ERROR(locked, "Cache Line for [Address(%#lx), REMOTE_WRITE_UNLOCK_REQ] unlocked", address);
      }
      return true;

   case ShmemMsg::READ_REPLY:
   case ShmemMsg::READ_LOCK_REPLY:
   case ShmemMsg::WRITE_REPLY:
   case ShmemMsg::WRITE_UNLOCK_REPLY:
      return false;

   default:
      LOG_PRINT_ERROR("Unrecognized Msg Type (%u)", msg_type);
      return false;
   }
}

void
L2CacheCntlr::releaseLock(ShmemMsg::Type msg_type, IntPtr address, bool locked)
{
   if (locked)
      _memory_manager->releaseLock();
   
   switch (msg_type)
   {
   case ShmemMsg::REMOTE_WRITE_UNLOCK_REQ:
      _memory_manager->wakeUpThreadAfterCacheLineUnlock(ShmemPerfModel::_APP_THREAD, address);
      break;
   default:
      break;
   }
}

tile_id_t
L2CacheCntlr::getTileID()
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
