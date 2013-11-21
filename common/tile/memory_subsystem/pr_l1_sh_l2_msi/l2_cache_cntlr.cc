#include "l2_cache_cntlr.h"
#include "memory_manager.h"
#include "directory_type.h"
#include "l2_directory_cfg.h"
#include "directory_entry.h"
#include "l2_cache_replacement_policy.h"
#include "l2_cache_hash_fn.h"
#include "config.h"
#include "log.h"

#define TYPE(shmem_req)    (shmem_req->getShmemMsg()->getType())

namespace PrL1ShL2MSI
{

L2CacheCntlr::L2CacheCntlr(MemoryManager* memory_manager,
                           AddressHomeLookup* dram_home_lookup,
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
   , _dram_home_lookup(dram_home_lookup)
   , _enabled(false)
{
   _L2_cache_replacement_policy_obj =
      new L2CacheReplacementPolicy(L2_cache_size, L2_cache_associativity, cache_line_size, _L2_cache_req_queue);
   _L2_cache_hash_fn_obj = new L2CacheHashFn(L2_cache_size, L2_cache_associativity, cache_line_size);

   // L2 cache
   _L2_cache = new Cache("L2",
         PR_L1_SH_L2_MSI,
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
}

L2CacheCntlr::~L2CacheCntlr()
{
   // Some eviction requests
   LOG_ASSERT_ERROR(_L2_cache_req_queue.size() == _evicted_cache_line_map.size(),
                    "Req list size(%u), Evicted cache line map size(%u)", _L2_cache_req_queue.size(), _evicted_cache_line_map.size());
   // FIXME: Directory Entries are not deleted at the end of simulation
   delete _L2_cache;
   delete _L2_cache_replacement_policy_obj;
   delete _L2_cache_hash_fn_obj;
}

void
L2CacheCntlr::getCacheLineInfo(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info, ShmemMsg::Type shmem_msg_type, bool update_miss_counters)
{
   map<IntPtr,ShL2CacheLineInfo>::iterator it = _evicted_cache_line_map.find(address);
   if (it == _evicted_cache_line_map.end())
   {
      assert(shmem_msg_type != ShmemMsg::NULLIFY_REQ);
      // Read it from the cache
      _L2_cache->getCacheLineInfo(address, L2_cache_line_info);
      if (update_miss_counters)
      {
         bool cache_miss = (L2_cache_line_info->getCState() == CacheState::INVALID);
         Core::mem_op_t mem_op_type = getMemOpTypeFromShmemMsgType(shmem_msg_type);
         _L2_cache->updateMissCounters(address, mem_op_type, cache_miss);
      }

      if (L2_cache_line_info->getCState() == CacheState::INVALID)
      {
         if ((shmem_msg_type == ShmemMsg::EX_REQ) || (shmem_msg_type == ShmemMsg::SH_REQ))
         {
            // allocate a cache line with garbage data
            allocateCacheLine(address, L2_cache_line_info);
         }
         else
         {
            LOG_PRINT_ERROR("Unrecognized shmem msg type(%u)", shmem_msg_type);
         }
      }
   }
   else // (present in the evicted map [_evicted_cache_line_map])
   {
      assert(!update_miss_counters);
      // Read it from the evicted cache line map
      L2_cache_line_info->assign(&it->second);
   }
}

void
L2CacheCntlr::setCacheLineInfo(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info)
{
   map<IntPtr,ShL2CacheLineInfo>::iterator it = _evicted_cache_line_map.find(address);
   if (it == _evicted_cache_line_map.end())
   {
      // Write it to the cache
      _L2_cache->setCacheLineInfo(address, L2_cache_line_info);
   }
   else
   {
      // Write it to the evicted cache line map
      (it->second).assign(L2_cache_line_info);
   }
}

void
L2CacheCntlr::readCacheLine(IntPtr address, Byte* data_buf)
{
  _L2_cache->accessCacheLine(address, Cache::LOAD, data_buf, getCacheLineSize());
}

void
L2CacheCntlr::writeCacheLine(IntPtr address, Byte* data_buf)
{
   _L2_cache->accessCacheLine(address, Cache::STORE, data_buf, getCacheLineSize());
}

void
L2CacheCntlr::allocateCacheLine(IntPtr address, ShL2CacheLineInfo* L2_cache_line_info)
{
   // Create the new directory entry
   DirectoryEntry* directory_entry = DirectoryEntry::create(PR_L1_SH_L2_MSI,
                                                            L2DirectoryCfg::getDirectoryType(),
                                                            L2DirectoryCfg::getMaxHWSharers(),
                                                            L2DirectoryCfg::getMaxNumSharers());
   // Construct meta-data info about L2 cache line
   *L2_cache_line_info = ShL2CacheLineInfo(_L2_cache->getTag(address), directory_entry);
   // Transition to a new cache state
   L2_cache_line_info->setCState(CacheState::DATA_INVALID);

   // Evicted Line Information
   bool eviction;
   IntPtr evicted_address;
   ShL2CacheLineInfo evicted_cache_line_info;
   Byte writeback_buf[getCacheLineSize()];

   _L2_cache->insertCacheLine(address, L2_cache_line_info, (Byte*) NULL,
                              &eviction, &evicted_address, &evicted_cache_line_info, writeback_buf);

   if (eviction)
   {
      assert(evicted_cache_line_info.isValid());
      LOG_ASSERT_ERROR(_L2_cache_req_queue.empty(evicted_address),
                       "Address(%#lx) is already being processed", evicted_address);
      LOG_ASSERT_ERROR(evicted_cache_line_info.getCState() == CacheState::CLEAN || evicted_cache_line_info.getCState() == CacheState::DIRTY,
                       "Cache Line State(%u)", evicted_cache_line_info.getCState());
      __attribute__((unused)) DirectoryEntry* evicted_directory_entry = evicted_cache_line_info.getDirectoryEntry();
      LOG_ASSERT_ERROR(evicted_directory_entry, "Cant find directory entry for address(%#lx)", evicted_address);

      bool msg_modeled = Config::getSingleton()->isApplicationTile(getTileId());
      Time eviction_time = getShmemPerfModel()->getCurrTime();
      
      LOG_PRINT("Eviction: Address(%#lx), Cache State(%u), Directory State(%u), Num Sharers(%i)",
                evicted_address, evicted_cache_line_info.getCState(),
                evicted_directory_entry->getDirectoryBlockInfo()->getDState(), evicted_directory_entry->getNumSharers());

      // Create a nullify req and add it onto the queue for processing
      ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::L2_CACHE, MemComponent::L2_CACHE,
                           getTileId(), false, evicted_address,
                           msg_modeled); 
      // Create a new ShmemReq for removing the sharers of the evicted cache line
      ShmemReq* nullify_req = new ShmemReq(&nullify_msg, eviction_time);
      // Insert the nullify_req into the set of requests to be processed
      _L2_cache_req_queue.enqueue(evicted_address, nullify_req);
      
      // Insert the evicted cache line info into the evicted cache line map for future reference
      _evicted_cache_line_map.insert(make_pair(evicted_address, evicted_cache_line_info));
     
      // Process the nullify req
      processNullifyReq(nullify_req, writeback_buf);
   }
   else
   {
      assert(!evicted_cache_line_info.isValid());
      assert(evicted_cache_line_info.getDirectoryEntry() == NULL);
   }
}

void
L2CacheCntlr::handleMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // add synchronization cost
   if (sender == getTileId()){
      getShmemPerfModel()->incrCurrTime(_L2_cache->getSynchronizationDelay(DVFSManager::convertToModule(shmem_msg->getSenderMemComponent())));
   }
   else{
      getShmemPerfModel()->incrCurrTime(_L2_cache->getSynchronizationDelay(NETWORK_MEMORY));
   }

   // Incr current time for every message that comes into the L2 cache
   _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   Time msg_time = getShmemPerfModel()->getCurrTime();
   IntPtr address = shmem_msg->getAddress();
   
   if ( (shmem_msg_type == ShmemMsg::EX_REQ) || (shmem_msg_type == ShmemMsg::SH_REQ) )
   {
      // Add request onto a queue
      ShmemReq* shmem_req = new ShmemReq(shmem_msg, msg_time);
      _L2_cache_req_queue.enqueue(address, shmem_req);

      if (_L2_cache_req_queue.count(address) == 1)
      {
         // Process the request
         processShmemReq(shmem_req);
      }
   }

   else if ( (shmem_msg_type == ShmemMsg::INV_REP) || (shmem_msg_type == ShmemMsg::FLUSH_REP) || (shmem_msg_type == ShmemMsg::WB_REP) )
   {
      // Get the ShL2CacheLineInfo object
      ShL2CacheLineInfo L2_cache_line_info;
      getCacheLineInfo(address, &L2_cache_line_info, shmem_msg_type);
      assert(L2_cache_line_info.isValid());

      if (L2_cache_line_info.getCachingComponent() != shmem_msg->getSenderMemComponent())
      {
         // LOG_PRINT_WARNING("Address(%#lx) removed from (L1-ICACHE)", address);
         LOG_ASSERT_ERROR(shmem_msg->getSenderMemComponent() == MemComponent::L1_ICACHE,
                          "Msg'ing component(%s)", SPELL_MEMCOMP(shmem_msg->getSenderMemComponent()));
         LOG_ASSERT_ERROR(L2_cache_line_info.getCachingComponent() == MemComponent::L1_DCACHE,
                          "Caching component(%s), Valid(%s), State(%s)",
                          SPELL_MEMCOMP(L2_cache_line_info.getCachingComponent()), L2_cache_line_info.isValid() ? "true" : "false",
                          SPELL_CSTATE(L2_cache_line_info.getCState()));
         assert(shmem_msg_type == ShmemMsg::INV_REP);
         // Drop the message
      }
      else // (L2_cache_line_info.getCachingComponent() == shmem_msg->getSenderMemComponent())
      {
         // I either find the cache line in the evicted_cache_line_map or in the L2 cache
         switch (shmem_msg_type)
         {
         case ShmemMsg::INV_REP:
            processInvRepFromL1Cache(sender, shmem_msg, &L2_cache_line_info);
            break;
         case ShmemMsg::FLUSH_REP:
            processFlushRepFromL1Cache(sender, shmem_msg, &L2_cache_line_info);
            break;
         case ShmemMsg::WB_REP:
            processWbRepFromL1Cache(sender, shmem_msg, &L2_cache_line_info);
            break;
         default:
            LOG_PRINT_ERROR("Unrecognized Shmem Msg Type: %u", shmem_msg_type);
            break;
         }
      }

      // Update the evicted cache line map or the L2 cache directly
      setCacheLineInfo(address, &L2_cache_line_info);

      // Get the latest request for the data (if any) and process it
      if (!_L2_cache_req_queue.empty(address))
      {
         ShmemReq* shmem_req = _L2_cache_req_queue.front(address);
         restartShmemReq(shmem_req, &L2_cache_line_info, shmem_msg->getDataBuf());
      }
   }

   else
   {
      LOG_PRINT_ERROR("Unrecognized shmem msg type(%u)", shmem_msg_type);
   }
}

void
L2CacheCntlr::handleMsgFromDram(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // Incr curr time for every message that comes into the L2 cache
   _memory_manager->incrCurrTime(MemComponent::L2_CACHE, CachePerfModel::ACCESS_DATA_AND_TAGS);

   IntPtr address = shmem_msg->getAddress();

   ShL2CacheLineInfo L2_cache_line_info;
   _L2_cache->getCacheLineInfo(address, &L2_cache_line_info);

   // Write the data into the L2 cache if it is a SH_REQ
   ShmemReq* shmem_req = _L2_cache_req_queue.front(address);
   if (TYPE(shmem_req) == ShmemMsg::SH_REQ)
      writeCacheLine(address, shmem_msg->getDataBuf());
   else
      LOG_ASSERT_ERROR(TYPE(shmem_req) == ShmemMsg::EX_REQ, "Type(%u)", TYPE(shmem_req));

   // Set Cache State to CLEAN (irrespective of whether the data is written)
   L2_cache_line_info.setCState(CacheState::CLEAN);
   // Write-back the cache line info
   setCacheLineInfo(address, &L2_cache_line_info);

   // Restart the shmem request
   restartShmemReq(shmem_req, &L2_cache_line_info, shmem_msg->getDataBuf());
}

void
L2CacheCntlr::processNextReqFromL1Cache(IntPtr address)
{
   LOG_PRINT("Start processNextReqFromL1Cache(%#lx)", address);
   
   // Add 1 cycle to denote that we are moving to the next request
   getShmemPerfModel()->incrCurrTime(Latency(1,_L2_cache->getFrequency()));

   assert(_L2_cache_req_queue.count(address) >= 1);
   
   // Get the completed shmem req
   ShmemReq* completed_shmem_req = _L2_cache_req_queue.dequeue(address);

   // Delete the completed shmem req
   delete completed_shmem_req;

   if (!_L2_cache_req_queue.empty(address))
   {
      LOG_PRINT("A new shmem req for address(%#lx) found", address);
      ShmemReq* shmem_req = _L2_cache_req_queue.front(address);

      // Update the Shared Mem current time appropriately
      shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      assert(TYPE(shmem_req) != ShmemMsg::NULLIFY_REQ);
      // Process the request
      processShmemReq(shmem_req);
   }

   LOG_PRINT("End processNextReqFromL1Cache(%#lx)", address);
}

void
L2CacheCntlr::processShmemReq(ShmemReq* shmem_req)
{
   ShmemMsg::Type msg_type = TYPE(shmem_req);

   // Process the request
   switch (msg_type)
   {
   case ShmemMsg::EX_REQ:
      processExReqFromL1Cache(shmem_req, NULL, true);
      break;
   case ShmemMsg::SH_REQ:
      processShReqFromL1Cache(shmem_req, NULL, true);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", TYPE(shmem_req));
      break;
   }
}

void
L2CacheCntlr::processNullifyReq(ShmemReq* nullify_req, Byte* data_buf)
{
   IntPtr address = nullify_req->getShmemMsg()->getAddress();
   tile_id_t requester = nullify_req->getShmemMsg()->getRequester();
   bool msg_modeled = nullify_req->getShmemMsg()->isModeled();

   // get cache line info 
   ShL2CacheLineInfo L2_cache_line_info;
   getCacheLineInfo(address, &L2_cache_line_info, ShmemMsg::NULLIFY_REQ);

   assert(L2_cache_line_info.getCState() != CacheState::INVALID);

   DirectoryEntry* directory_entry = L2_cache_line_info.getDirectoryEntry();
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   // Is the request completely processed or waiting for acknowledgements or data?
   bool completed = false;

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         assert(L2_cache_line_info.getCachingComponent() == MemComponent::L1_DCACHE);
         ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::L2_CACHE, MemComponent::L1_DCACHE,
                            requester, false, address,
                            msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg);
      }
      break;

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
                          "Address(%#lx), State(SHARED), owner(%i)", address, directory_entry->getOwner());
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, 
                          "Address(%#lx), Directory State(SHARED), Num Sharers(%u)",
                          address, directory_entry->getNumSharers());
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendInvalidationMsg(ShmemMsg::NULLIFY_REQ,
                             address, L2_cache_line_info.getCachingComponent(),
                             all_tiles_sharers, sharers_list,
                             requester, msg_modeled);

         // Send line to DRAM_CNTLR if dirty
         if ((L2_cache_line_info.getCState() == CacheState::DIRTY) && data_buf)
            storeDataInDram(address, data_buf, requester, msg_modeled);
      }
      break;

   case DirectoryState::UNCACHED:
      {
         // Send line to DRAM_CNTLR if dirty
         if ((L2_cache_line_info.getCState() == CacheState::DIRTY) && data_buf)
            storeDataInDram(address, data_buf, requester, msg_modeled);

         // Set completed to true
         completed = true;
      }
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }

   if (completed)
   {
      // Write the cache line info back
      setCacheLineInfo(address, &L2_cache_line_info);

      // Delete the directory entry
      delete L2_cache_line_info.getDirectoryEntry();
      // Remove the address from the evicted map since its handling is complete
      _evicted_cache_line_map.erase(address);

      // Process the next request if completed
      processNextReqFromL1Cache(address);
   }
}

void
L2CacheCntlr::processExReqFromL1Cache(ShmemReq* shmem_req, Byte* data_buf, bool first_call)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   __attribute__((unused)) MemComponent::Type requester_mem_component = shmem_req->getShmemMsg()->getSenderMemComponent();
   assert(requester_mem_component == MemComponent::L1_DCACHE);
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();

   ShL2CacheLineInfo L2_cache_line_info;
   getCacheLineInfo(address, &L2_cache_line_info, ShmemMsg::EX_REQ, first_call);
 
   assert(L2_cache_line_info.getCState() != CacheState::INVALID);

   // Is the request completely processed or waiting for acknowledgements or data?
   bool completed = false;

   if (L2_cache_line_info.getCState() != CacheState::DATA_INVALID)
   {
      // Data need not be fetched from DRAM
      // The cache line is present in the L2 cache
      DirectoryEntry* directory_entry = L2_cache_line_info.getDirectoryEntry();
      DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

      switch (curr_dstate)
      {
      case DirectoryState::MODIFIED:
         {
            // Flush the owner
            LOG_ASSERT_ERROR(directory_entry->getOwner() != INVALID_TILE_ID,
                             "Address(%#lx), State(MODIFIED), owner(INVALID)", address);
            LOG_ASSERT_ERROR(L2_cache_line_info.getCachingComponent() == MemComponent::L1_DCACHE,
                             "Caching component(%u)", L2_cache_line_info.getCachingComponent());

            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::L2_CACHE, MemComponent::L1_DCACHE,
                               requester, false, address,
                               msg_modeled);
            _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg);
         }
         break;

      case DirectoryState::SHARED:
         {
            LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
                             "Address(%#lx), State(SHARED), owner(%i)", address, directory_entry->getOwner());
            LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, 
                             "Address(%#lx), Directory State(SHARED), Num Sharers(%u)",
                             address, directory_entry->getNumSharers());
            LOG_ASSERT_ERROR(L2_cache_line_info.getCachingComponent() == MemComponent::L1_DCACHE,
                             "Caching component(%u)", L2_cache_line_info.getCachingComponent());
      
            if ((directory_entry->hasSharer(requester)) && (directory_entry->getNumSharers() == 1))
            {
               // Upgrade miss - shortcut - set state to MODIFIED
               directory_entry->setOwner(requester);
               directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::MODIFIED);

               ShmemMsg shmem_msg(ShmemMsg::UPGRADE_REP, MemComponent::L2_CACHE, MemComponent::L1_DCACHE,
                                  requester, false, address,
                                  msg_modeled);
               _memory_manager->sendMsg(requester, shmem_msg);
               
               // Set completed to true
               completed = true;
            }
            else
            {
               // Invalidate all the sharers
               vector<tile_id_t> sharers_list;
               bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
              
               sendInvalidationMsg(ShmemMsg::EX_REQ,
                                   address, MemComponent::L1_DCACHE,
                                   all_tiles_sharers, sharers_list,
                                   requester, msg_modeled);
            }
         }
         break;

      case DirectoryState::UNCACHED:
         {
            assert(directory_entry->getNumSharers() == 0);
          
            // Set caching component 
            L2_cache_line_info.setCachingComponent(MemComponent::L1_DCACHE);

            // Add the sharer and set that as the owner 
            __attribute__((unused)) bool add_result = directory_entry->addSharer(requester);
            assert(add_result);
            directory_entry->setOwner(requester);
            directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::MODIFIED);

            readCacheLineAndSendToL1Cache(ShmemMsg::EX_REP, address, MemComponent::L1_DCACHE, data_buf, requester, msg_modeled);

            // Set completed to true
            completed = true;
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized DirectoryState(%u)", curr_dstate);
         break;
      }
   }

   else // (!L2_cache_line_info->getCState() == CacheState::DATA_INVALID)
   {
      // Cache line is not present in the L2 cache
      // Send a message to the memory controller asking it to fetch the line from DRAM
      fetchDataFromDram(address, requester, msg_modeled);
   }

   if (completed)
   {
      // Write the cache line info back
      setCacheLineInfo(address, &L2_cache_line_info);

      // Process the next request if completed
      processNextReqFromL1Cache(address);
   }
}

void
L2CacheCntlr::processShReqFromL1Cache(ShmemReq* shmem_req, Byte* data_buf, bool first_call)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   // Get the requesting mem component (L1-I/L1-D cache)
   MemComponent::Type requester_mem_component = shmem_req->getShmemMsg()->getSenderMemComponent();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();

   ShL2CacheLineInfo L2_cache_line_info;
   getCacheLineInfo(address, &L2_cache_line_info, ShmemMsg::SH_REQ, first_call);
 
   assert(L2_cache_line_info.getCState() != CacheState::INVALID);

   // Is the request completely processed or waiting for acknowledgements or data?
   bool completed = false;

   if (L2_cache_line_info.getCState() != CacheState::DATA_INVALID)
   {
      // Data need not be fetched from DRAM
      // The cache line is present in the L2 cache
      DirectoryEntry* directory_entry = L2_cache_line_info.getDirectoryEntry();
      DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

      switch (curr_dstate)
      {
      case DirectoryState::MODIFIED:
         {
            LOG_ASSERT_ERROR(directory_entry->getOwner() != INVALID_TILE_ID,
                             "Address(%#lx), State(MODIFIED), owner(INVALID)", address);
            LOG_ASSERT_ERROR(L2_cache_line_info.getCachingComponent() == requester_mem_component,
                             "caching component(%u), requester component(%u)",
                             L2_cache_line_info.getCachingComponent(), requester_mem_component);

            ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::L2_CACHE, MemComponent::L1_DCACHE,
                               requester, false, address,
                               msg_modeled);
            _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg);
         }
         break;

      case DirectoryState::SHARED:
         {
            LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, "Address(%#lx), State(%u), Num Sharers(%u)",
                             address, curr_dstate, directory_entry->getNumSharers());
            
            if (L2_cache_line_info.getCachingComponent() != requester_mem_component)
            {
               assert(directory_entry->hasSharer(requester));
               
               // LOG_PRINT_WARNING("Address(%#lx) cached first in (%s), then in (%s)",
               //                   address, SPELL_MEMCOMP(L2_cache_line_info.getCachingComponent()),
               //                   SPELL_MEMCOMP(requester_mem_component));
               L2_cache_line_info.setCachingComponent(MemComponent::L1_DCACHE);
               
               // Read the cache-line from the L2 cache and send it to L1
               readCacheLineAndSendToL1Cache(ShmemMsg::SH_REP, address, requester_mem_component, data_buf, requester, msg_modeled);
               // set completed to true 
               completed = true;
               break;
            }

            LOG_ASSERT_ERROR(L2_cache_line_info.getCachingComponent() == requester_mem_component,
                             "Address(%#lx), Num sharers(%i), caching component(%u), requester component(%u)",
                             address, directory_entry->getNumSharers(),
                             L2_cache_line_info.getCachingComponent(), requester_mem_component);

            // Try to add the sharer to the sharer list
            bool add_result = directory_entry->addSharer(requester);
            if (add_result == false)
            {
               // Invalidate one sharer
               tile_id_t sharer_id = directory_entry->getOneSharer();

               // Invalidate the sharer
               ShmemMsg shmem_msg(ShmemMsg::INV_REQ, MemComponent::L2_CACHE, requester_mem_component,
                                  requester, false, address,
                                  msg_modeled);
               _memory_manager->sendMsg(sharer_id, shmem_msg);
            }
            else // succesfully added the sharer
            {
               // Read the cache-line from the L2 cache and send it to L1
               readCacheLineAndSendToL1Cache(ShmemMsg::SH_REP, address, requester_mem_component, data_buf, requester, msg_modeled);
            
               // set completed to true 
               completed = true; 
            }
         }
         break;

      case DirectoryState::UNCACHED:
         {
            assert(directory_entry->getNumSharers() == 0);
          
            // Set caching component 
            L2_cache_line_info.setCachingComponent(requester_mem_component);
            
            // Modifiy the directory entry contents
            __attribute__((unused)) bool add_result = directory_entry->addSharer(requester);
            LOG_ASSERT_ERROR(add_result, "Address(%#lx), Requester(%i), State(UNCACHED), Num Sharers(%u)",
                             address, requester, directory_entry->getNumSharers());
            directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::SHARED);

            // Read the cache-line from the L2 cache and send it to L1
            readCacheLineAndSendToL1Cache(ShmemMsg::SH_REP, address, requester_mem_component, data_buf, requester, msg_modeled);
       
            // set completed to true 
            completed = true; 
         }
         break;

      default:
         LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
         break;
      }
   }
   
   else // (!L2_cache_line_info->getCState() == CacheState::DATA_INVALID)
   {
      // Cache line is not present in the L2 cache
      // Send a message to the memory controller asking it to fetch the line from DRAM
      fetchDataFromDram(address, requester, msg_modeled);
   }

   if (completed)
   {
      // Write the cache line info back
      setCacheLineInfo(address, &L2_cache_line_info);

      // Process the next request if completed
      processNextReqFromL1Cache(address);
   }
}

void
L2CacheCntlr::processInvRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info)
{
   __attribute__((unused)) IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
  
   switch (curr_dstate)
   {
   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
                       "Address(%#lx), State(SHARED), sender(%i), num sharers(%u), owner(%i)",
                       address, sender, directory_entry->getNumSharers(), directory_entry->getOwner());

      // Remove the sharer and set the directory state to UNCACHED if the number of sharers is 0
      directory_entry->removeSharer(sender, shmem_msg->isReplyExpected());
      if (directory_entry->getNumSharers() == 0)
      {
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::MODIFIED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), INV_REP, State(%u), num sharers(%u), owner(%i)",
                      address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }
}

void
L2CacheCntlr::processFlushRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   
   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), State(MODIFIED), sender(%i), owner(%i)",
                          address, sender, directory_entry->getOwner());

         assert(!shmem_msg->isReplyExpected());

         // Write the line to the L2 cache if there is no request (or a SH_REQ)
         ShmemReq* shmem_req = _L2_cache_req_queue.front(address);
         if ( (shmem_req == NULL) || (TYPE(shmem_req) == ShmemMsg::SH_REQ) )
         {
            writeCacheLine(address, shmem_msg->getDataBuf());
         }
         // Set the line to dirty even if it is logically so
         L2_cache_line_info->setCState(CacheState::DIRTY);

         // Remove the sharer from the directory entry and set state to UNCACHED
         directory_entry->removeSharer(sender, false);
         directory_entry->setOwner(INVALID_TILE_ID);
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::SHARED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), FLUSH_REP, Sender(%i), Sender mem component(%u), State(%u), num sharers(%u), owner(%i)",
                      address, sender, shmem_msg->getSenderMemComponent(),
                      curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }
}

void
L2CacheCntlr::processWbRepFromL1Cache(tile_id_t sender, const ShmemMsg* shmem_msg, ShL2CacheLineInfo* L2_cache_line_info)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   assert(!shmem_msg->isReplyExpected());

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
         LOG_ASSERT_ERROR(!_L2_cache_req_queue.empty(address),
                          "Address(%#lx), WB_REP, req queue empty!!", address);

         // Write the data into the L2 cache and mark the cache line dirty
         writeCacheLine(address, shmem_msg->getDataBuf());
         L2_cache_line_info->setCState(CacheState::DIRTY);

         // Set the directory state to SHARED
         directory_entry->setOwner(INVALID_TILE_ID);
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::SHARED);
      }
      break;

   case DirectoryState::SHARED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#llx), WB_REP, State(%u), num sharers(%u), owner(%i)",
                      address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }
}

void
L2CacheCntlr::restartShmemReq(ShmemReq* shmem_req, ShL2CacheLineInfo* L2_cache_line_info, Byte* data_buf)
{
   // Add 1 cycle to denote that we are restarting the request
   getShmemPerfModel()->incrCurrTime(Latency(1, _L2_cache->getFrequency()));

   // Update ShmemReq & ShmemPerfModel internal time
   shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
   getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

   DirectoryEntry* directory_entry = L2_cache_line_info->getDirectoryEntry();
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   ShmemMsg::Type msg_type = TYPE(shmem_req);
   switch (msg_type)
   {
   case ShmemMsg::EX_REQ:
      if (curr_dstate == DirectoryState::UNCACHED)
         processExReqFromL1Cache(shmem_req, data_buf);
      break;

   case ShmemMsg::SH_REQ:
      processShReqFromL1Cache(shmem_req, data_buf);
      break;

   case ShmemMsg::NULLIFY_REQ:
      if (curr_dstate == DirectoryState::UNCACHED)
         processNullifyReq(shmem_req, data_buf);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized request type(%u)", msg_type);
      break;
   }
}

void
L2CacheCntlr::sendInvalidationMsg(ShmemMsg::Type requester_msg_type,
                                  IntPtr address, MemComponent::Type receiver_mem_component,
                                  bool all_tiles_sharers, vector<tile_id_t>& sharers_list,
                                  tile_id_t requester, bool msg_modeled)
{
   if (all_tiles_sharers)
   {
      bool reply_expected = (L2DirectoryCfg::getDirectoryType() == LIMITED_BROADCAST);
      // Broadcast invalidation request to all tiles 
      // (irrespective of whether they are sharers or not)
      ShmemMsg shmem_msg(ShmemMsg::INV_REQ, MemComponent::L2_CACHE, receiver_mem_component, 
                         requester, reply_expected, address,
                         msg_modeled);
      _memory_manager->broadcastMsg(shmem_msg);
   }
   else // not all tiles are sharers
   {
      // Send Invalidation Request to only a specific set of sharers
      for (UInt32 i = 0; i < sharers_list.size(); i++)
      {
         ShmemMsg shmem_msg(ShmemMsg::INV_REQ, MemComponent::L2_CACHE, receiver_mem_component,
                            requester, false, address,
                            msg_modeled);
         _memory_manager->sendMsg(sharers_list[i], shmem_msg);
      }
   }
}

void
L2CacheCntlr::readCacheLineAndSendToL1Cache(ShmemMsg::Type reply_msg_type,
                                            IntPtr address, MemComponent::Type requester_mem_component,
                                            Byte* data_buf,
                                            tile_id_t requester, bool msg_modeled)
{
   if (data_buf != NULL)
   {
      // I already have the data I need cached (by reply from an owner)
      ShmemMsg shmem_msg(reply_msg_type, MemComponent::L2_CACHE, requester_mem_component,
                         requester, false, address, 
                         data_buf, getCacheLineSize(),
                         msg_modeled);
      _memory_manager->sendMsg(requester, shmem_msg);
   }
   else
   {
      // Read the data from L2 cache
      Byte L2_data_buf[getCacheLineSize()];
      readCacheLine(address, L2_data_buf);
      
      ShmemMsg shmem_msg(reply_msg_type, MemComponent::L2_CACHE, requester_mem_component,
                         requester, false, address,
                         L2_data_buf, getCacheLineSize(),
                         msg_modeled);
      _memory_manager->sendMsg(requester, shmem_msg); 
   }
}

void
L2CacheCntlr::fetchDataFromDram(IntPtr address, tile_id_t requester, bool msg_modeled)
{
   ShmemMsg fetch_msg(ShmemMsg::DRAM_FETCH_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_CNTLR,
                      requester, false, address,
                      msg_modeled);
   _memory_manager->sendMsg(getDramHome(address), fetch_msg);
}

void
L2CacheCntlr::storeDataInDram(IntPtr address, Byte* data_buf, tile_id_t requester, bool msg_modeled)
{
   ShmemMsg send_msg(ShmemMsg::DRAM_STORE_REQ, MemComponent::L2_CACHE, MemComponent::DRAM_CNTLR,
                     requester, false, address,
                     data_buf, getCacheLineSize(),
                     msg_modeled);
   _memory_manager->sendMsg(getDramHome(address), send_msg);
}

Core::mem_op_t
L2CacheCntlr::getMemOpTypeFromShmemMsgType(ShmemMsg::Type shmem_msg_type)
{
   switch (shmem_msg_type)
   {
   case ShmemMsg::SH_REQ:
      return Core::READ;
   case ShmemMsg::EX_REQ:
      return Core::WRITE;
   default:
      LOG_PRINT_ERROR("Unrecognized Msg(%u)", shmem_msg_type);
      return Core::READ;                 
   }
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
