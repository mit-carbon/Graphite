using namespace std;

#include "dram_directory_cntlr.h"
#include "log.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectoryMSI
{

DramDirectoryCntlr::DramDirectoryCntlr(MemoryManager* memory_manager,
      DramCntlr* dram_cntlr,
      string dram_directory_total_entries_str,
      UInt32 dram_directory_associativity,
      UInt32 cache_line_size,
      UInt32 dram_directory_max_num_sharers,
      UInt32 dram_directory_max_hw_sharers,
      string dram_directory_type_str,
      string dram_directory_access_cycles_str,
      UInt32 num_dram_cntlrs)
   : _memory_manager(memory_manager)
   , _dram_cntlr(dram_cntlr)
{
   _dram_directory_cache = new DirectoryCache(_memory_manager->getTile(),
                                              PR_L1_PR_L2_DRAM_DIRECTORY_MSI,
                                              dram_directory_type_str,
                                              dram_directory_total_entries_str,
                                              dram_directory_associativity,
                                              cache_line_size,
                                              dram_directory_max_hw_sharers,
                                              dram_directory_max_num_sharers,
                                              num_dram_cntlrs,
                                              dram_directory_access_cycles_str,
                                              getShmemPerfModel());

   LOG_PRINT("Instantiated Dram Directory Cache");
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
               if (shmem_msg_type == ShmemMsg::EX_REQ)
                  processExReqFromL2Cache(shmem_req);
               else if (shmem_msg_type == ShmemMsg::SH_REQ)
                  processShReqFromL2Cache(shmem_req);
               else
                  LOG_PRINT_ERROR("Unrecognized Request(%u)", shmem_msg_type);
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
   ShmemReq* completed_shmem_req = _dram_directory_req_queue.dequeue(address);
   delete completed_shmem_req;

   if (! _dram_directory_req_queue.empty(address))
   {
      LOG_PRINT("A new shmem req for address(%#lx) found", address);
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);

      // Update the Shared Mem current time appropriately
      shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getType() == ShmemMsg::EX_REQ)
         processExReqFromL2Cache(shmem_req);
      else if (shmem_req->getShmemMsg()->getType() == ShmemMsg::SH_REQ)
         processShReqFromL2Cache(shmem_req);
      else
         LOG_PRINT_ERROR("Unrecognized Request(%u)", shmem_req->getShmemMsg()->getType());
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

   // The NULLIFY requests are always modeled in the network
   bool msg_modeled = true;
   ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::DRAM_DIRECTORY, requester, replaced_address, msg_modeled);

   ShmemReq* nullify_req = new ShmemReq(&nullify_msg, msg_time);
   _dram_directory_req_queue.enqueue(replaced_address, nullify_req);

   assert(_dram_directory_req_queue.count(replaced_address) == 1);
   processNullifyReq(nullify_req);

   return directory_entry;
}

void
DramDirectoryCntlr::processNullifyReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();
   
   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         ShmemMsg msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                      msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), msg);
      }
      break;

   case DirectoryState::SHARED:

      {
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         if (all_tiles_sharers)
         {
            // Broadcast Invalidation Request to all tiles 
            // (irrespective of whether they are sharers or not)
            ShmemMsg msg(ShmemMsg::INV_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                         msg_modeled);
            _memory_manager->broadcastMsg(msg);
         }
         else
         {
            // Send Invalidation Request to only a specific set of sharers
            for (UInt32 i = 0; i < sharers_list.size(); i++)
            {
               ShmemMsg msg(ShmemMsg::INV_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                            msg_modeled);
               _memory_manager->sendMsg(sharers_list[i], msg);
            }
         }
      }
      break;

   case DirectoryState::UNCACHED:

      {
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
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();
   
   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {   
         assert(cached_data_buf == NULL);
         ShmemMsg msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                      msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), msg);
      }
      break;

   case DirectoryState::SHARED:

      {
         assert(cached_data_buf == NULL);
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         if (all_tiles_sharers)
         {
            // Broadcast Invalidation Request to all tiles 
            // (irrespective of whether they are sharers or not)
            ShmemMsg msg(ShmemMsg::INV_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                         msg_modeled);
            _memory_manager->broadcastMsg(msg);
         }
         else
         {
            // Send Invalidation Request to only a specific set of sharers
            for (UInt32 i = 0; i < sharers_list.size(); i++)
            {
               ShmemMsg msg(ShmemMsg::INV_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                            msg_modeled);
               _memory_manager->sendMsg(sharers_list[i], msg);
            }
         }
      }
      break;

   case DirectoryState::UNCACHED:
      
      {
         // Modifiy the directory entry contents
         __attribute__((unused)) bool add_result = directory_entry->addSharer(requester);
         assert(add_result);
         directory_entry->setOwner(requester);
         directory_block_info->setDState(DirectoryState::MODIFIED);

         retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, requester, address, cached_data_buf, msg_modeled);

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
DramDirectoryCntlr::processShReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   bool msg_modeled = shmem_req->getShmemMsg()->isModeled();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         assert(cached_data_buf == NULL);
         ShmemMsg msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                      msg_modeled);
         _memory_manager->sendMsg(directory_entry->getOwner(), msg);
      }
      break;

   case DirectoryState::SHARED:
      {
         bool add_result = directory_entry->addSharer(requester);
         if (add_result == false)
         {
            tile_id_t sharer_id = directory_entry->getOneSharer();
            // Send a message to another sharer to invalidate that
            ShmemMsg msg(ShmemMsg::INV_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, requester, address,
                         msg_modeled);
            _memory_manager->sendMsg(sharer_id, msg);
         }
         else
         {
            retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf, msg_modeled);
   
            // Process Next Request
            processNextReqFromL2Cache(address);
         }
      }
      break;

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         __attribute__((unused)) bool add_result = directory_entry->addSharer(requester);
         assert(add_result);
         directory_block_info->setDState(DirectoryState::SHARED);

         retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf, msg_modeled);
   
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
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(ShmemMsg::Type reply_msg_type,
      tile_id_t receiver, IntPtr address, Byte* cached_data_buf, bool msg_modeled)
{
   if (cached_data_buf != NULL)
   {
      // I already have the data I need cached
      ShmemMsg msg(reply_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, receiver, address,
                   cached_data_buf, getCacheLineSize(), msg_modeled);
      _memory_manager->sendMsg(receiver, msg);
   }
   else
   {
      // Get the data from DRAM
      // This could be directly forwarded to the cache or passed
      // through the Dram Directory Controller

      // I have to get the data from DRAM
      Byte data_buf[getCacheLineSize()];
      
      _dram_cntlr->getDataFromDram(address, data_buf, msg_modeled);
      
      ShmemMsg msg(reply_msg_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, receiver, address,
                   data_buf, getCacheLineSize(), msg_modeled);
      _memory_manager->sendMsg(receiver, msg);
   }
}

void
DramDirectoryCntlr::processInvRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::SHARED);

   directory_entry->removeSharer(sender);
   if (directory_entry->getNumSharers() == 0)
   {
      directory_block_info->setDState(DirectoryState::UNCACHED);
   }

   if (_dram_directory_req_queue.count(address) > 0)
   {
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);

      // Update Times in the Shmem Perf Model and the Shmem Req
      shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getType() == ShmemMsg::EX_REQ)
      {
         // An ShmemMsg::EX_REQ caused the invalidation
         if (directory_block_info->getDState() == DirectoryState::UNCACHED)
         {
            processExReqFromL2Cache(shmem_req);
         }
      }
      else if (shmem_req->getShmemMsg()->getType() == ShmemMsg::SH_REQ)
      {
         // A ShmemMsg::SH_REQ caused the invalidation
         processShReqFromL2Cache(shmem_req);
      }
      else // shmem_req->getShmemMsg()->getType() == ShmemMsg::NULLIFY_REQ
      {
         if (directory_block_info->getDState() == DirectoryState::UNCACHED)
         {
            processNullifyReq(shmem_req);
         }
      }
   }
}

void
DramDirectoryCntlr::processFlushRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::MODIFIED);

   directory_entry->removeSharer(sender);
   directory_entry->setOwner(INVALID_TILE_ID);
   directory_block_info->setDState(DirectoryState::UNCACHED);

   if (_dram_directory_req_queue.count(address) != 0)
   {
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);

      // Update times
      shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      // An involuntary/voluntary Flush
      if (shmem_req->getShmemMsg()->getType() == ShmemMsg::EX_REQ)
      {
         processExReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
      else if (shmem_req->getShmemMsg()->getType() == ShmemMsg::SH_REQ)
      {
         // Write Data to Dram
         sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());
         processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
      else // shmem_req->getShmemMsg()->getType() == ShmemMsg::NULLIFY_REQ
      {
         // Write Data To Dram
         sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());
         processNullifyReq(shmem_req);
      }
   }
   else
   {
      // This was just an eviction
      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());
   }
}

void
DramDirectoryCntlr::processWbRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);
   
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();

   assert(directory_block_info->getDState() == DirectoryState::MODIFIED);
   assert(directory_entry->hasSharer(sender));
   
   directory_entry->setOwner(INVALID_TILE_ID);
   directory_block_info->setDState(DirectoryState::SHARED);

   if (_dram_directory_req_queue.count(address) != 0)
   {
      ShmemReq* shmem_req = _dram_directory_req_queue.front(address);

      // Update Time
      shmem_req->updateTime(getShmemPerfModel()->getCurrTime());
      getShmemPerfModel()->updateCurrTime(shmem_req->getTime());

      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getDataBuf(), shmem_msg->isModeled());

      LOG_ASSERT_ERROR(shmem_req->getShmemMsg()->getType() == ShmemMsg::SH_REQ,
            "Address(0x%x), Req(%u)",
            address, shmem_req->getShmemMsg()->getType());
      processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
   }
   else
   {
      LOG_PRINT_ERROR("Should not reach here");
   }
}

void
DramDirectoryCntlr::sendDataToDram(IntPtr address, Byte* data_buf, bool modeled)
{
   // Write data to Dram
   _dram_cntlr->putDataToDram(address, data_buf, modeled);
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

}
