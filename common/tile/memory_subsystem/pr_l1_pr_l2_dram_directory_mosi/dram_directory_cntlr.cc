#include <cstring>
#include "dram_directory_cntlr.h"
#include "memory_manager.h"
#include "core_model.h"
#include "utils.h"
#include "config.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

DramDirectoryCntlr::DramDirectoryCntlr(MemoryManager* memory_manager,
      DramCntlr* dram_cntlr,
      string dram_directory_total_entries_str,
      UInt32 dram_directory_associativity,
      UInt32 cache_line_size,
      UInt32 dram_directory_max_num_sharers,
      UInt32 dram_directory_max_hw_sharers,
      string dram_directory_type_str,
      UInt32 num_dram_cntlrs,
      string dram_directory_access_cycles_str)
   : _memory_manager(memory_manager)
   , _dram_cntlr(dram_cntlr)
   , _enabled(false)
{
   _dram_directory_cache = new DirectoryCache(_memory_manager->getTile(),
                                              PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
                                              dram_directory_type_str,
                                              dram_directory_total_entries_str,
                                              dram_directory_associativity,
                                              cache_line_size,
                                              dram_directory_max_hw_sharers,
                                              dram_directory_max_num_sharers,
                                              num_dram_cntlrs,
                                              dram_directory_access_cycles_str,
                                              getShmemPerfModel());

   _directory_type = DirectoryEntry::parseDirectoryType(dram_directory_type_str);

   // Update Counters
   initializeEventCounters();
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete _dram_directory_cache;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   // add synchronization cost
   if (sender == _memory_manager->getTile()->getId()){
      getShmemPerfModel()->incrCurrTime(_dram_directory_cache->getSynchronizationDelay(L2_CACHE));
   }
   else{
      getShmemPerfModel()->incrCurrTime(_dram_directory_cache->getSynchronizationDelay(NETWORK_MEMORY));
   }

   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   Time msg_time = getShmemPerfModel()->getCurrTime();

   switch (shmem_msg_type)
   {
   case ShmemMsg::EX_REQ:
   case ShmemMsg::SH_REQ:

      {
         IntPtr address = shmem_msg->getAddress();
         
         // Add request onto a queue
         ShmemReq* shmem_req = new ShmemReq(shmem_msg, msg_time);
         _dram_directory_req_queue.enqueue(address, shmem_req);

         if (_dram_directory_req_queue.count(address) == 1)
         {
            // The req is processed immediately
            shmem_req->updateProcessingStartTime(msg_time);

            // No data should be cached for this address
            assert(_cached_data_list.lookup(address) == NULL);
            
            switch (shmem_msg_type)
            {
            case ShmemMsg::EX_REQ:
               processExReqFromL2Cache(shmem_req, (DirectoryEntry*) NULL, true);
               break;
            case ShmemMsg::SH_REQ:
               processShReqFromL2Cache(shmem_req, (DirectoryEntry*) NULL, true);
               break;
            default:
               LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", shmem_msg_type);
               break;
            }
         }
      }
      break;

   case ShmemMsg::INV_REP:
      processInvRepFromL2Cache(sender, shmem_msg);
      break;

   case ShmemMsg::FLUSH_REP:
      processFlushRepFromL2Cache(sender, shmem_msg);
      break;

   case ShmemMsg::WB_REP:
      processWbRepFromL2Cache(sender, shmem_msg);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Msg Type: %u", shmem_msg_type);
      break;
   }
}

void
DramDirectoryCntlr::processNextReqFromL2Cache(IntPtr address)
{
   LOG_PRINT("Start processNextReqFromL2Cache(%#lx)", address);

   assert(_dram_directory_req_queue.count(address) >= 1);
   
   // Get the completed shmem req
   ShmemReq* completed_shmem_req = _dram_directory_req_queue.dequeue(address);

   // Update Finish time
   completed_shmem_req->updateProcessingFinishTime(getShmemPerfModel()->getCurrTime());

   // Update latency counters
   updateShmemReqLatencyCounters(completed_shmem_req);

   // Delete the completed shmem req
   delete completed_shmem_req;

   // No longer should any data be cached for this address
   assert(_cached_data_list.lookup(address) == NULL);

   if (!_dram_directory_req_queue.empty(address))
   {
      LOG_PRINT("A new shmem req for address(%#lx) found", address);
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);

      // Update the Shared Mem time appropriately
      shmem_req->updateProcessingStartTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      switch (shmem_req->getShmemMsg()->getType())
      {
      case ShmemMsg::EX_REQ:
         processExReqFromL2Cache(shmem_req, (DirectoryEntry*) NULL, true);
         break;
      case ShmemMsg::SH_REQ:
         processShReqFromL2Cache(shmem_req, (DirectoryEntry*) NULL, true);
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", shmem_req->getShmemMsg()->getType());
         break;
      }
   }

   LOG_PRINT("End processNextReqFromL2Cache(%#lx)", address);
}

DirectoryEntry*
DramDirectoryCntlr::processDirectoryEntryAllocationReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   Time msg_time = getShmemPerfModel()->getCurrTime();

   std::vector<DirectoryEntry*> replacement_candidate_list;
   _dram_directory_cache->getReplacementCandidates(address, replacement_candidate_list);

   std::vector<DirectoryEntry*>::iterator it;
   std::vector<DirectoryEntry*>::iterator replacement_candidate = replacement_candidate_list.end();
   for (it = replacement_candidate_list.begin(); it != replacement_candidate_list.end(); it++)
   {
      if ( ( (replacement_candidate == replacement_candidate_list.end()) ||
             ((*replacement_candidate)->getNumSharers() > (*it)->getNumSharers()) 
           )
           &&
           (_dram_directory_req_queue.count((*it)->getAddress()) == 0)
         )
      {
         replacement_candidate = it;
      }
   }

   LOG_ASSERT_ERROR(replacement_candidate != replacement_candidate_list.end(),
         "Cant find a directory entry to be replaced with a non-zero request list");

   IntPtr replaced_address = (*replacement_candidate)->getAddress();

   // We get the entry with the lowest number of sharers
   DirectoryEntry* directory_entry = _dram_directory_cache->replaceDirectoryEntry(replaced_address, address);

   bool msg_modeled = true;
   ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::DRAM_DIRECTORY,
                        requester, INVALID_TILE_ID, false, replaced_address, msg_modeled);

   ShmemReq* nullify_req = new ShmemReq(&nullify_msg, msg_time);
   _dram_directory_req_queue.enqueue(replaced_address, nullify_req);

   assert(_dram_directory_req_queue.count(replaced_address) == 1);
   processNullifyReq(nullify_req, (DirectoryEntry*) NULL, true);

   return directory_entry;
}

void
DramDirectoryCntlr::processNullifyReq(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();
  
   if (!directory_entry)
      directory_entry = _dram_directory_cache->getDirectoryEntry(address);

   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   if (first_call)
   {
      shmem_req->setInitialDState(curr_dstate);
      shmem_req->setInitialBroadcastMode(directory_entry->inBroadcastMode());
      // Update Counters
      updateShmemReqEventCounters(shmem_req, directory_entry);
   }

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
               requester, INVALID_TILE_ID, false, address,
               msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg); 
      }
      break;

   case DirectoryState::OWNED:
      {
         LOG_ASSERT_ERROR(directory_entry->getOwner() != INVALID_TILE_ID,
               "Address(0x%x), State(OWNED), owner(%i)", address, directory_entry->getOwner());

         // FLUSH_REQ to Owner
         // INV_REQ to all sharers except owner (Also sent to the Owner for sake of convenience)
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendShmemMsg(ShmemMsg::NULLIFY_REQ, ShmemMsg::INV_FLUSH_COMBINED_REQ, address, requester, 
               directory_entry->getOwner(), all_tiles_sharers, sharers_list,
               msg_modeled);
      }
      break;

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
               "Address(0x%x), State(SHARED), owner(%i)", address, directory_entry->getOwner());
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendShmemMsg(ShmemMsg::NULLIFY_REQ, ShmemMsg::INV_REQ, address, requester, 
               INVALID_TILE_ID, all_tiles_sharers, sharers_list,
               msg_modeled);
      }
      break;

   case DirectoryState::UNCACHED:
      {
         // Send data to Dram
         Byte* cached_data_buf = _cached_data_list.lookup(address);
         if (cached_data_buf != NULL)
         {
            sendDataToDram(address, cached_data_buf, msg_modeled);
            _cached_data_list.erase(address);
         }

         _dram_directory_cache->invalidateDirectoryEntry(address);
         
         // Process Next Request
         processNextReqFromL2Cache(address);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }
}

void
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();
  
   if (!directory_entry)
      directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   if (first_call)
   {
      shmem_req->setInitialDState(curr_dstate);
      shmem_req->setInitialBroadcastMode(directory_entry->inBroadcastMode());
      // Update Counters
      updateShmemReqEventCounters(shmem_req, directory_entry);
   }

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         // FLUSH_REQ to Owner
         ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
               requester, INVALID_TILE_ID, false, address,
               msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg);
      }
      break;

   case DirectoryState::OWNED:
      {
         if ((directory_entry->getOwner() == requester) && (directory_entry->getNumSharers() == 1))
         {
            directory_block_info->setDState(DirectoryState::MODIFIED);

            ShmemMsg shmem_msg(ShmemMsg::UPGRADE_REP, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                  requester, INVALID_TILE_ID, false, address, msg_modeled);
            _memory_manager->sendMsg(requester, shmem_msg);
           
            processNextReqFromL2Cache(address);
         }
         else
         {
            // FLUSH_REQ to Owner
            // INV_REQ to all sharers except owner (Also sent to the Owner for sake of convenience)
            
            vector<tile_id_t> sharers_list;
            bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
            
            sendShmemMsg(ShmemMsg::EX_REQ, ShmemMsg::INV_FLUSH_COMBINED_REQ, address, requester, 
                  directory_entry->getOwner(), all_tiles_sharers, sharers_list,
                  msg_modeled);
         }
      }
      break;

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, 
               "Address(0x%x), Directory State(SHARED), Num Sharers(%u)",
               address, directory_entry->getNumSharers());
         
         if ((directory_entry->hasSharer(requester)) && (directory_entry->getNumSharers() == 1))
         {
            directory_entry->setOwner(requester);
            directory_block_info->setDState(DirectoryState::MODIFIED);

            ShmemMsg shmem_msg(ShmemMsg::UPGRADE_REP, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                  requester, INVALID_TILE_ID, false, address, msg_modeled);
            _memory_manager->sendMsg(requester, shmem_msg);
            
            processNextReqFromL2Cache(address);
         }
         else
         {
            // getOneSharer() is a deterministic function
            // FLUSH_REQ to One Sharer (If present)
            // INV_REQ to all other sharers
            
            vector<tile_id_t> sharers_list;
            bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
           
            sendShmemMsg(ShmemMsg::EX_REQ, ShmemMsg::INV_FLUSH_COMBINED_REQ, address, requester,
                  directory_entry->getOneSharer(), all_tiles_sharers, sharers_list,
                  msg_modeled);
         }
      }
      break;

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() == 0,
               "Address(%#lx), State(UNCACHED), Num Sharers(%u)",
               address, directory_entry->getNumSharers());

         __attribute__((unused)) bool add_result = addSharer(directory_entry, requester);
         LOG_ASSERT_ERROR(add_result, "Address(%#lx), State(UNCACHED)", address);
         
         directory_entry->setOwner(requester);
         directory_block_info->setDState(DirectoryState::MODIFIED);

         retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, requester, address, msg_modeled);

         // Process Next Request
         processNextReqFromL2Cache(address);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }
}

void
DramDirectoryCntlr::processShReqFromL2Cache(ShmemReq* shmem_req, DirectoryEntry* directory_entry, bool first_call)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();

   if (!directory_entry)
      directory_entry = _dram_directory_cache->getDirectoryEntry(address);

   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   if (first_call)
   {
      shmem_req->setInitialDState(curr_dstate);
      shmem_req->setInitialBroadcastMode(directory_entry->inBroadcastMode());
      // Update Counters
      updateShmemReqEventCounters(shmem_req, directory_entry);
   }

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
               requester, INVALID_TILE_ID, false, address,
               msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), shmem_msg);

         shmem_req->setSharerTileId(directory_entry->getOwner());
      }
      break;

   case DirectoryState::OWNED:
   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0,
               "Address(%#lx), State(%u), Num Sharers(%u)",
               address, curr_dstate, directory_entry->getNumSharers());

         // Fetch data from one sharer - Sharer can be the owner too !!
         tile_id_t sharer_id = directory_entry->getOneSharer();

         bool add_result = addSharer(directory_entry, requester);
         if (add_result == false)
         {
            LOG_ASSERT_ERROR(sharer_id != INVALID_TILE_ID, "Address(%#lx), SH_REQ, state(%u), sharer(%i)",
                  address, curr_dstate, sharer_id);

            // Send a message to the sharer to write back the data and also to invalidate itself
            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                  requester, INVALID_TILE_ID, false, address,
                  msg_modeled);
            _memory_manager->sendMsg(sharer_id, shmem_msg);

            shmem_req->setSharerTileId(sharer_id);
         }
         else
         {
            Byte* cached_data_buf = _cached_data_list.lookup(address);
            if ((cached_data_buf == NULL) && (sharer_id != INVALID_TILE_ID))
            {
               // Remove the added sharer since the request has not been completed
               removeSharer(directory_entry, requester, false);

               // Get data from one of the sharers
               ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                     requester, INVALID_TILE_ID, false, address,
                     msg_modeled);
               _memory_manager->sendMsg(sharer_id, shmem_msg);

               shmem_req->setSharerTileId(sharer_id);
            }
            else
            {
               retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, msg_modeled);
      
               // Process Next Request
               processNextReqFromL2Cache(address);
            }
         }
      }
      break;

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         __attribute__((unused)) bool add_result = addSharer(directory_entry, requester);
         LOG_ASSERT_ERROR(add_result, "Address(%#lx), Requester(%i), State(UNCACHED), Num Sharers(%u)",
                          address, requester, directory_entry->getNumSharers());
         
         directory_block_info->setDState(DirectoryState::SHARED);

         retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, msg_modeled);
  
         // Process Next Request
         processNextReqFromL2Cache(address);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }
}

void
DramDirectoryCntlr::sendShmemMsg(ShmemMsg::Type requester_msg_type, ShmemMsg::Type send_msg_type, IntPtr address,
      tile_id_t requester, tile_id_t single_receiver, bool all_tiles_sharers, vector<tile_id_t>& sharers_list, bool msg_modeled)
{
   if (all_tiles_sharers)
   {
      bool reply_expected = false;
      if (_directory_type == LIMITED_BROADCAST)
         reply_expected = true;

      // Broadcast Invalidation Request to all tiles 
      // (irrespective of whether they are sharers or not)
      ShmemMsg shmem_msg(send_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, 
            requester, single_receiver, reply_expected, address, msg_modeled);
      _memory_manager->broadcastMsg(shmem_msg);
   }
   else
   {
      // Send Invalidation Request to only a specific set of sharers
      for (UInt32 i = 0; i < sharers_list.size(); i++)
      {
         ShmemMsg shmem_msg(send_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
               requester, single_receiver, false, address, msg_modeled);
         _memory_manager->sendMsg(sharers_list[i], shmem_msg);
      }
   }
}

void
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(ShmemMsg::Type reply_msg_type,
      tile_id_t receiver, IntPtr address, bool msg_modeled)
{
   Byte* cached_data_buf = _cached_data_list.lookup(address);

   if (cached_data_buf != NULL)
   {
      // I already have the data I need cached
      ShmemMsg shmem_msg(reply_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
            receiver, INVALID_TILE_ID, false, address, 
            cached_data_buf, getCacheLineSize(), msg_modeled);
      _memory_manager->sendMsg(receiver, shmem_msg);

      _cached_data_list.erase(address);
   }
   else
   {
      // Get the data from DRAM
      // This could be directly forwarded to the cache or passed
      // through the Dram Directory Controller

      // I have to get the data from DRAM
      Byte data_buf[getCacheLineSize()];
      
      _dram_cntlr->getDataFromDram(address, data_buf, msg_modeled);
      
      ShmemMsg shmem_msg(reply_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
            receiver, INVALID_TILE_ID, false, address,
            data_buf, getCacheLineSize(), msg_modeled);
      _memory_manager->sendMsg(receiver, shmem_msg); 
   }
}

void
DramDirectoryCntlr::processInvRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();
  
   switch (curr_dstate)
   {
   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR((sender != directory_entry->getOwner()) && (directory_entry->getNumSharers() > 0),
            "Address(%#lx), State(OWNED), num sharers(%u), sender(%i), owner(%i)",
            address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      assert(directory_entry->getNumSharers() > 0);
      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
            "Address(%#lx), State(SHARED), num sharers(%u), sender(%i), owner(%i)",
            address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      if (directory_entry->getNumSharers() == 0)
         directory_block_info->setDState(DirectoryState::UNCACHED);
      break;

   case DirectoryState::MODIFIED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), INV_REP, State(%u), num sharers(%u), owner(%i)",
            address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (_dram_directory_req_queue.count(address) > 0)
   {
      // Get the latest request for the data
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);
      restartShmemReq(sender, shmem_req, directory_entry);
   }
}

void
DramDirectoryCntlr::processFlushRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();
   
   LOG_ASSERT_ERROR(curr_dstate != DirectoryState::UNCACHED,
         "Address(%#lx), State(%u)", address, curr_dstate);

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
            "Address(%#lx), State(MODIFIED), sender(%i), owner(%i)",
            address, sender, directory_entry->getOwner());

      assert(! shmem_msg->isReplyExpected());
      removeSharer(directory_entry, sender, false);
      directory_entry->setOwner(INVALID_TILE_ID);
      directory_block_info->setDState(DirectoryState::UNCACHED); 
      break;

   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() != INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
            "Address(%#lx), State(OWNED), owner(%i), num sharers(%u)",
            address, directory_entry->getOwner(), directory_entry->getNumSharers());
      
      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      if (sender == directory_entry->getOwner())
      {
         directory_entry->setOwner(INVALID_TILE_ID);
         if (directory_entry->getNumSharers() > 0)
            directory_block_info->setDState(DirectoryState::SHARED);
         else
            directory_block_info->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
            "Address(%#lx), State(SHARED), owner(%i), num sharers(%u)",
            address, directory_entry->getOwner(), directory_entry->getNumSharers());

      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      if (directory_entry->getNumSharers() == 0)
         directory_block_info->setDState(DirectoryState::UNCACHED);
      break;

   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), FLUSH_REP, State(%u), num sharers(%u), owner(%i)",
            address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (_dram_directory_req_queue.count(address) > 0)
   {
      // First save the data in one of the buffers in the directory cntlr
      _cached_data_list.insert(address, shmem_msg->getDataBuf(), getCacheLineSize());

      // Get the latest request for the data
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);
     
      // Write-back to memory in certain circumstances
      if (shmem_req->getShmemMsg()->getType() == ShmemMsg::SH_REQ)
      {
         DirectoryState::Type initial_dstate = curr_dstate;
         DirectoryState::Type final_dstate = directory_block_info->getDState();

         if ((initial_dstate == DirectoryState::MODIFIED || initial_dstate == DirectoryState::OWNED)
            && (final_dstate == DirectoryState::SHARED || final_dstate == DirectoryState::UNCACHED))
         {
            sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());
         }
      }

      restartShmemReq(sender, shmem_req, directory_entry);
   }
   else
   {
      // This was just an eviction
      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());
   }
}

void
DramDirectoryCntlr::processWbRepFromL2Cache(tile_id_t sender, const ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);
   
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   assert(! shmem_msg->isReplyExpected());

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
            "Address(%#lx), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
      LOG_ASSERT_ERROR(_dram_directory_req_queue.count(address) > 0,
            "Address(%#lx), WB_REP, req queue empty!!", address);

      directory_block_info->setDState(DirectoryState::OWNED);
      break;

   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR(directory_entry->hasSharer(sender),
            "Address(%#lx), sender(%i), NOT sharer", address, sender);

      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
            "Address(%#lx), State(%u), Owner(%i)", address, directory_entry->getOwner());
      LOG_ASSERT_ERROR(directory_entry->hasSharer(sender),
            "Address(%#lx), sender(%i), NOT sharer", address, sender);

      break;

   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#llx), WB_REP, State(%u), num sharers(%u), owner(%i)",
            address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (_dram_directory_req_queue.count(address) > 0)
   {
      // First save the data in one of the buffers in the directory cntlr
      _cached_data_list.insert(address, shmem_msg->getDataBuf(), getCacheLineSize());

      // Get the latest request for the data
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);
      restartShmemReq(sender, shmem_req, directory_entry);
   }
   else
   {
      // Ignore the data since this data is already present in the caches of some tiles
      LOG_PRINT_ERROR("Address(%#lx), WB_REP, NO requester", address);
   }
}

void 
DramDirectoryCntlr::restartShmemReq(tile_id_t sender, ShmemReq* shmem_req, DirectoryEntry* directory_entry)
{
   // Update Request & ShmemPerfModel times
   shmem_req->updateProcessingFinishTime(getShmemPerfModel()->getCurrTime());
   getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

   ShmemMsg::Type msg_type = shmem_req->getShmemMsg()->getType();

   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   switch(msg_type)
   {
   case ShmemMsg::EX_REQ:
      if (curr_dstate == DirectoryState::UNCACHED)
         processExReqFromL2Cache(shmem_req, directory_entry);
      break;

   case ShmemMsg::SH_REQ:
      assert(shmem_req->getSharerTileId() != INVALID_TILE_ID);
      if (sender == shmem_req->getSharerTileId())
      {
         shmem_req->setSharerTileId(INVALID_TILE_ID);
         processShReqFromL2Cache(shmem_req, directory_entry);
      }
      break;

   case ShmemMsg::NULLIFY_REQ:
      if (curr_dstate == DirectoryState::UNCACHED)
         processNullifyReq(shmem_req, directory_entry);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized request type(%u)", msg_type);
      break;
   }
}

void
DramDirectoryCntlr::sendDataToDram(IntPtr address, Byte* data_buf, bool msg_modeled)
{
   // Write data to Dram
   _dram_cntlr->putDataToDram(address, data_buf, msg_modeled);
}

void
DramDirectoryCntlr::initializeEventCounters()
{
   // EX_REQ counters
   _total_exreq = 0;
   _total_exreq_in_modified_state = 0;
   _total_exreq_in_shared_state = 0;
   _total_exreq_with_upgrade_replies = 0;
   _total_exreq_in_uncached_state = 0;
   _total_exreq_serialization_time = Time(0);
   _total_exreq_processing_time = Time(0);

   // SH_REQ counters
   _total_shreq = 0;
   _total_shreq_in_modified_state = 0;
   _total_shreq_in_shared_state = 0;
   _total_shreq_in_uncached_state = 0;
   _total_shreq_serialization_time = Time(0);
   _total_shreq_processing_time = Time(0);
   
   // NULLIFY_REQ counters
   _total_nullifyreq = 0;
   _total_nullifyreq_in_modified_state = 0;
   _total_nullifyreq_in_shared_state = 0;
   _total_nullifyreq_in_uncached_state = 0;
   _total_nullifyreq_serialization_time = Time(0);
   _total_nullifyreq_processing_time = Time(0);

   // Invalidation Counters
   _total_invalidations_unicast_mode = 0;
   _total_invalidations_broadcast_mode = 0;
   _total_sharers_invalidated_unicast_mode = 0;
   _total_sharers_invalidated_broadcast_mode = 0;
   _total_invalidation_processing_time_unicast_mode = Time(0);
   _total_invalidation_processing_time_broadcast_mode = Time(0);
}

void
DramDirectoryCntlr::updateShmemReqEventCounters(ShmemReq* shmem_req, DirectoryEntry* directory_entry)
{
   if (!_enabled)
      return;

   ShmemMsg::Type shmem_msg_type = shmem_req->getShmemMsg()->getType();
   DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   switch (shmem_msg_type)
   {
   case ShmemMsg::EX_REQ:
      _total_exreq ++;
      switch (dstate)
      {
      case DirectoryState::MODIFIED:
         _total_exreq_in_modified_state ++;
         break;
      case DirectoryState::OWNED:
      case DirectoryState::SHARED:
         _total_exreq_in_shared_state ++;
         if ((directory_entry->getNumSharers() == 1) && (directory_entry->getOneSharer() == requester))
         {
            shmem_req->setUpgradeReply();
            _total_exreq_with_upgrade_replies ++;
         }
         else
         {
            updateInvalidationEventCounters(directory_entry->inBroadcastMode(), directory_entry->getNumSharers());
         }
         break;
      case DirectoryState::UNCACHED:
         _total_exreq_in_uncached_state ++;
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized Directory State (%u)", dstate);
         break;
      }
      break;

   case ShmemMsg::SH_REQ:
      _total_shreq ++;
      switch (dstate)
      {
      case DirectoryState::MODIFIED:
         _total_shreq_in_modified_state ++;
         break;
      case DirectoryState::OWNED:
      case DirectoryState::SHARED:
         _total_shreq_in_shared_state ++;
         break;
      case DirectoryState::UNCACHED:
         _total_shreq_in_uncached_state ++;
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized Directory State (%u)", dstate);
         break;
      }
      break;

   case ShmemMsg::NULLIFY_REQ:
      _total_nullifyreq ++;
      switch (dstate)
      {
      case DirectoryState::MODIFIED:
         _total_nullifyreq_in_modified_state ++;
         break;
      case DirectoryState::OWNED:
      case DirectoryState::SHARED:
         _total_nullifyreq_in_shared_state ++;
         updateInvalidationEventCounters(directory_entry->inBroadcastMode(), directory_entry->getNumSharers());
         break;
      case DirectoryState::UNCACHED:
         _total_nullifyreq_in_uncached_state ++;
         break;
      default:
         LOG_PRINT_ERROR("Unrecognized Directory State (%u)", dstate);
         break;
      }
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Msg type(%u)", shmem_msg_type);
      break;
   }
}

void
DramDirectoryCntlr::updateInvalidationEventCounters(bool in_broadcast_mode, SInt32 num_sharers)
{
   if (in_broadcast_mode)
   {
      _total_invalidations_broadcast_mode ++;
      _total_sharers_invalidated_broadcast_mode += num_sharers;
   }
   else // unicast mode
   {
      _total_invalidations_unicast_mode ++;
      _total_sharers_invalidated_unicast_mode += num_sharers;
   }
}

void
DramDirectoryCntlr::updateShmemReqLatencyCounters(const ShmemReq* shmem_req)
{
   if (!_enabled)
      return;

   ShmemMsg::Type shmem_req_type = shmem_req->getShmemMsg()->getType();
   DirectoryState::Type initial_dstate = shmem_req->getInitialDState();
   bool initial_broadcast_mode = shmem_req->getInitialBroadcastMode();

   switch (shmem_req_type)
   {
   case ShmemMsg::EX_REQ:
      _total_exreq_serialization_time += shmem_req->getSerializationTime();
      _total_exreq_processing_time += shmem_req->getProcessingTime();
      if ( ((initial_dstate == DirectoryState::OWNED) || (initial_dstate == DirectoryState::SHARED)) && (!shmem_req->isUpgradeReply()) )
         updateInvalidationLatencyCounters(initial_broadcast_mode, shmem_req->getProcessingTime());
      break;
   case ShmemMsg::SH_REQ:
      _total_shreq_serialization_time += shmem_req->getSerializationTime();
      _total_shreq_processing_time += shmem_req->getProcessingTime();
      break;
   case ShmemMsg::NULLIFY_REQ:
      _total_nullifyreq_serialization_time += shmem_req->getSerializationTime();
      _total_nullifyreq_processing_time += shmem_req->getProcessingTime();
      if ((initial_dstate == DirectoryState::OWNED) || (initial_dstate == DirectoryState::SHARED))
         updateInvalidationLatencyCounters(initial_broadcast_mode, shmem_req->getProcessingTime());
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Req Type(%u)", shmem_req_type);
      break;
   }
}

void
DramDirectoryCntlr::updateInvalidationLatencyCounters(bool initial_broadcast_mode, Time invalidation_processing_time)
{
   if (initial_broadcast_mode)
      _total_invalidation_processing_time_broadcast_mode += invalidation_processing_time;
   else // unicast mode
      _total_invalidation_processing_time_unicast_mode += invalidation_processing_time;
}

bool
DramDirectoryCntlr::addSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id)
{
   if (_directory_type == FULL_MAP)
      _dram_directory_cache->getDirectory()->updateSharerStats(directory_entry->getNumSharers(), directory_entry->getNumSharers() + 1);
   return directory_entry->addSharer(sharer_id);
}

void
DramDirectoryCntlr::removeSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id, bool reply_expected)
{
   if (_directory_type == FULL_MAP)
      _dram_directory_cache->getDirectory()->updateSharerStats(directory_entry->getNumSharers(), directory_entry->getNumSharers() - 1);
   directory_entry->removeSharer(sharer_id, reply_expected);
}

void
DramDirectoryCntlr::outputSummary(ostream& out)
{
   out << "Dram Directory Cntlr: " << endl;
   out << "    Total Requests: " << _total_exreq + _total_shreq + _total_nullifyreq << endl;
   out << "    Exclusive Requests: " << _total_exreq << endl; 
   out << "    Shared Requests: " << _total_shreq << endl;
   out << "    Nullify Requests: " << _total_nullifyreq << endl;

   if (_total_exreq > 0)
   {
      out << "    Exclusive Request - MODIFIED State: " << _total_exreq_in_modified_state << endl;
      out << "    Exclusive Request - SHARED State: " << _total_exreq_in_shared_state << endl;
      out << "    Exclusive Request - UNCACHED State: " << _total_exreq_in_uncached_state << endl;
      out << "    Exclusive Request - Upgrade Reply: " << _total_exreq_with_upgrade_replies << endl;

      out << "    Average Exclusive Request Serialization Time (in nanoseconds): "
          << ((float) _total_exreq_serialization_time.toNanosec()) / ((float) _total_exreq) << endl;
      out << "    Average Exclusive Request Processing Time (in nanoseconds): "
          << ((float) _total_exreq_processing_time.toNanosec()) / ((float) _total_exreq) << endl;
   }
   else
   {
      out << "    Exclusive Request - MODIFIED State: " << endl;
      out << "    Exclusive Request - SHARED State: " << endl;
      out << "    Exclusive Request - UNCACHED State: " << endl;
      out << "    Exclusive Request - Upgrade Reply: " << endl;

      out << "    Average Exclusive Request Serialization Time (in nanoseconds): " << endl;
      out << "    Average Exclusive Request Processing Time (in nanoseconds): " << endl;
   }

   if (_total_shreq > 0)
   {
      out << "    Shared Request - MODIFIED State: " << _total_shreq_in_modified_state << endl;
      out << "    Shared Request - SHARED State: " << _total_shreq_in_shared_state << endl;
      out << "    Shared Request - UNCACHED State: " << _total_shreq_in_uncached_state << endl;

      out << "    Average Shared Request Serialization Time (in nanoseconds): "
          <<  ((float) _total_shreq_serialization_time.toNanosec()) / ((float) _total_shreq) << endl;
      out << "    Average Shared Request Processing Time (in nanoseconds): "
          << ((float) _total_shreq_processing_time.toNanosec()) / ((float) _total_shreq) << endl;
   }
   else
   {
      out << "    Shared Request - MODIFIED State: " << endl;
      out << "    Shared Request - SHARED State: " << endl;
      out << "    Shared Request - UNCACHED State: " << endl;

      out << "    Average Shared Request Serialization Time (in nanoseconds): " << endl;
      out << "    Average Shared Request Processing Time (in nanoseconds): " << endl;
   }

   if (_total_nullifyreq > 0)
   {
      out << "    Nullify Request - MODIFIED State: " << _total_nullifyreq_in_modified_state << endl;
      out << "    Nullify Request - SHARED State: " << _total_nullifyreq_in_shared_state << endl;
      out << "    Nullify Request - UNCACHED State: " << _total_nullifyreq_in_uncached_state << endl;

      out << "    Average Nullify Request Serialization Time (in nanoseconds): "
          <<  ((float)_total_nullifyreq_serialization_time.toNanosec()) / ((float) _total_nullifyreq) << endl;
      out << "    Average Nullify Request Processing Time (in nanoseconds): "
          <<  ((float)_total_nullifyreq_processing_time.toNanosec()) / ((float) _total_nullifyreq) << endl;
   }
   else
   {
      out << "    Nullify Request - MODIFIED State: " << endl;
      out << "    Nullify Request - SHARED State: " << endl;
      out << "    Nullify Request - UNCACHED State: " << endl;

      out << "    Average Nullify Request Serialization Time (in nanoseconds): " << endl;
      out << "    Average Nullify Request Processing Time (in nanoseconds): " << endl;
   }

   out << "    Total Invalidation Requests - Unicast Mode: " << _total_invalidations_unicast_mode << endl;
   if (_total_invalidations_unicast_mode > 0)
   {
      out << "    Average Sharers Invalidated - Unicast Mode: "
          << ((float) _total_sharers_invalidated_unicast_mode) / ((float) _total_invalidations_unicast_mode) << endl;
      out << "    Average Invalidation Processing Time - Unicast Mode (in nanoseconds): "
          << ((float) _total_invalidation_processing_time_unicast_mode.toNanosec()) / ((float) _total_invalidations_unicast_mode) << endl;
   }
   else
   {
      out << "    Average Sharers Invalidated - Unicast Mode: " << endl;
      out << "    Average Invalidation Processing Time - Unicast Mode (in nanoseconds): " << endl;
   }

   out << "    Total Invalidation Requests - Broadcast Mode: " << _total_invalidations_broadcast_mode << endl;
   if (_total_invalidations_broadcast_mode > 0)
   {
      out << "    Average Sharers Invalidated - Broadcast Mode: "
         << ((float) _total_sharers_invalidated_broadcast_mode) / ((float)_total_invalidations_broadcast_mode) << endl;
      out << "    Average Invalidation Processing Time - Broadcast Mode (in nanoseconds): "
         << ((float)_total_invalidation_processing_time_broadcast_mode.toNanosec()) / ((float) _total_invalidations_broadcast_mode) << endl;
   }
   else
   {
      out << "    Average Sharers Invalidated - Broadcast Mode: " << endl;
      out << "    Average Invalidation Processing Time - Broadcast Mode (in nanoseconds): " << endl;
   }
}

void
DramDirectoryCntlr::dummyOutputSummary(ostream& out)
{
   out << "Dram Directory Cntlr: " << endl;
   out << "    Total Requests: " << endl;
   out << "    Exclusive Requests: " << endl; 
   out << "    Shared Requests: " << endl;
   out << "    Nullify Requests: " << endl;
  
   out << "    Exclusive Request - MODIFIED State: " << endl;
   out << "    Exclusive Request - SHARED State: " << endl;
   out << "    Exclusive Request - UNCACHED State: " << endl;
   out << "    Exclusive Request - Upgrade Reply: " << endl;

   out << "    Average Exclusive Request Serialization Time (in nanoseconds): " << endl;
   out << "    Average Exclusive Request Processing Time (in nanoseconds): " << endl;

   out << "    Shared Request - MODIFIED State: " << endl;
   out << "    Shared Request - SHARED State: " << endl;
   out << "    Shared Request - UNCACHED State: " << endl;

   out << "    Average Shared Request Serialization Time (in nanoseconds): " << endl;
   out << "    Average Shared Request Processing Time (in nanoseconds): " << endl;

   out << "    Nullify Request - MODIFIED State: " << endl;
   out << "    Nullify Request - SHARED State: " << endl;
   out << "    Nullify Request - UNCACHED State: " << endl;

   out << "    Average Nullify Request Serialization Time (in nanoseconds): " << endl;
   out << "    Average Nullify Request Processing Time (in nanoseconds): " << endl;

   out << "    Total Invalidation Requests - Unicast Mode: " << endl;
   out << "    Average Sharers Invalidated - Unicast Mode: " << endl;
   out << "    Average Invalidation Processing Time - Unicast Mode (in nanoseconds): " << endl;

   out << "    Total Invalidation Requests - Broadcast Mode: " << endl;
   out << "    Average Sharers Invalidated - Broadcast Mode: " << endl;
   out << "    Average Invalidation Processing Time - Broadcast Mode (in nanoseconds): " << endl;
}

UInt32
DramDirectoryCntlr::getCacheLineSize()
{
   return _memory_manager->getCacheLineSize();
}

ShmemPerfModel*
DramDirectoryCntlr::getShmemPerfModel()
{
   return _memory_manager->getShmemPerfModel();
}

DramDirectoryCntlr::DataList::DataList()
{}

DramDirectoryCntlr::DataList::~DataList()
{}

void
DramDirectoryCntlr::DataList::insert(IntPtr address, Byte* data, UInt32 size)
{
   Byte* alloc_data = new Byte[size];
   memcpy(alloc_data, data, size);

   pair<map<IntPtr,Byte*>::iterator, bool> ret = _data_list.insert(make_pair<IntPtr,Byte*>(address, alloc_data));
   if (ret.second == false)
   {
      // There is already some data present
      __attribute__((unused)) SInt32 equal = memcmp(alloc_data, (ret.first)->second, size);
      LOG_ASSERT_ERROR(equal == 0, "Address(%#lx), cached data different from now received data");
      delete [] alloc_data;
   }
   
   LOG_ASSERT_ERROR(_data_list.size() <= Config::getSingleton()->getTotalTiles(),
                    "_data_list.size() = %u, _total_tiles = %u",
                    _data_list.size(), Config::getSingleton()->getTotalTiles());
}

Byte*
DramDirectoryCntlr::DataList::lookup(IntPtr address)
{
   std::map<IntPtr,Byte*>::iterator data_list_it = _data_list.find(address);
   if (data_list_it != _data_list.end())
      return (data_list_it->second);
   else
      return ((Byte*) NULL);
}

void
DramDirectoryCntlr::DataList::erase(IntPtr address)
{
   std::map<IntPtr,Byte*>::iterator data_list_it = _data_list.find(address);
   LOG_ASSERT_ERROR(data_list_it != _data_list.end(),
         "Unable to erase address(0x%x) from _data_list", address);

   delete [] (data_list_it->second);
   _data_list.erase(data_list_it);
}

}
