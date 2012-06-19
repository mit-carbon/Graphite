#include "dram_directory_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

#define TYPE(req)    (req->getShmemMsg()->getType())

DramDirectoryCntlr::DramDirectoryCntlr(MemoryManager* memory_manager,
                                       AddressHomeLookup* dram_home_lookup,
                                       UInt32 directory_total_entries,
                                       UInt32 directory_associativity,
                                       UInt32 cache_line_size,
                                       UInt32 directory_max_num_sharers,
                                       UInt32 directory_max_hw_sharers,
                                       string directory_type,
                                       UInt32 num_directory_slices,
                                       UInt64 directory_access_delay)
   : _memory_manager(memory_manager)
   , _dram_home_lookup(dram_home_lookup)
   , _enabled(false)
{
   _directory = new DirectoryCache(_memory_manager->getTile(),
                                   HYBRID_PROTOCOL__PP_MOSI__SS,
                                   directory_type,
                                   directory_total_entries,
                                   directory_associativity,
                                   cache_line_size,
                                   directory_max_hw_sharers,
                                   directory_max_num_sharers,
                                   num_directory_slices,
                                   directory_access_delay);

   _directory_req_queue_list = new HashMapQueue<IntPtr,ShmemReq*>();

   _directory_type = DirectoryEntry::parseDirectoryType(directory_type);
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete _directory_req_queue_list;
   delete _directory;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   // Update mode information
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(shmem_msg->getAddress());
   Classifier* classifier = dynamic_cast<AdaptiveDirectoryEntry*>(directory_entry)->getClassifier();
   classifier->updateMode(sender, shmem_msg, directory_entry);

   switch (shmem_msg_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      {
         // Add request onto a queue
         IntPtr address = shmem_msg->getAddress();
         UInt64 msg_time = getShmemPerfModel()->getCycleCount();
         ShmemReq* directory_req = new ShmemReq(shmem_msg, msg_time);
         _directory_req_queue_list->enqueue(address, directory_req);

         if (_directory_req_queue_list->count(address) == 1)
         {
            processUnifiedReqFromL2Cache(directory_req, directory_entry);
         }
      }
      break;

   case ShmemMsg::WRITE_UNLOCK_REQ:
      processWriteUnlockReqFromL2Cache(sender, shmem_msg);
      break;

   // Messages from the L2 cache to the directory for the MOSI protocol
   case ShmemMsg::INV_REPLY:
      processInvReplyFromL2Cache(sender, shmem_msg);
      break;

   case ShmemMsg::FLUSH_REPLY:
      processFlushReplyFromL2Cache(sender, shmem_msg);
      break;

   case ShmemMsg::WB_REPLY:
      processWbReplyFromL2Cache(sender, shmem_msg);
      break;

   // Messages from the L2 cache to the directory due to the Remote-Access/Remote-Store protocols
   case ShmemMsg::REMOTE_READ_REPLY:
   case ShmemMsg::REMOTE_READ_LOCK_REPLY:
   case ShmemMsg::REMOTE_WRITE_REPLY:
   case ShmemMsg::REMOTE_WRITE_UNLOCK_REPLY:
      processRemoteAccessReplyFromL2Cache(sender, shmem_msg);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", shmem_msg_type);
      break;
   }
}

void
DramDirectoryCntlr::handleMsgFromDramCntlr(tile_id_t sender, ShmemMsg* dram_reply)
{
   ShmemMsg::Type dram_reply_type = dram_reply->getType();

   switch (dram_reply_type)
   {
   case ShmemMsg::DRAM_FETCH_REPLY:
      processDramFetchReply(sender, dram_reply);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", dram_reply_type);
      break;
   }
}

void
DramDirectoryCntlr::processNextReqFromL2Cache(IntPtr address)
{
   LOG_PRINT("Start processNextReqFromL2Cache(%#lx)", address);

   assert(_directory_req_queue_list->count(address) >= 1);
   ShmemReq* completed_directory_req = _directory_req_queue_list->dequeue(address);
   delete completed_directory_req;

   if (!_directory_req_queue_list->empty(address))
   {
      LOG_PRINT("A new shmem req for address(%#lx) found", address);
      ShmemReq* directory_req = _directory_req_queue_list->front(address);
      
      // Update the Shared Mem Cycle Counts
      directory_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(directory_req->getTime());

      processUnifiedReqFromL2Cache(directory_req);
   }

   LOG_PRINT("End processNextReqFromL2Cache(%#lx)", address);
}

DirectoryEntry*
DramDirectoryCntlr::processDirectoryEntryAllocationReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   tile_id_t requester = shmem_req->getShmemMsg()->getRequester();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   std::vector<DirectoryEntry*> replacement_candidate_list;
   _directory->getReplacementCandidates(address, replacement_candidate_list);

   std::vector<DirectoryEntry*>::iterator it;
   std::vector<DirectoryEntry*>::iterator replacement_candidate = replacement_candidate_list.end();
   for (it = replacement_candidate_list.begin(); it != replacement_candidate_list.end(); it++)
   {
      if ( ( (replacement_candidate == replacement_candidate_list.end()) ||
             ((*replacement_candidate)->getNumSharers() > (*it)->getNumSharers()) 
           )
           &&
           (_directory_req_queue_list->count((*it)->getAddress()) == 0)
         )
      {
         replacement_candidate = it;
      }
   }

   LOG_ASSERT_ERROR(replacement_candidate != replacement_candidate_list.end(),
         "Cant find a directory entry to be replaced with a non-zero request list");

   IntPtr replaced_address = (*replacement_candidate)->getAddress();

   // We get the entry with the lowest number of sharers
   DirectoryEntry* directory_entry = _directory->replaceDirectoryEntry(replaced_address, address);

   bool msg_modeled = ::MemoryManager::isMissTypeModeled(Cache::CAPACITY_MISS);
   ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::DRAM_DIRECTORY,
                        replaced_address,
                        requester, msg_modeled);

   ShmemReq* nullify_req = new ShmemReq(&nullify_msg, msg_time);
   _directory_req_queue_list->enqueue(replaced_address, nullify_req);

   assert(_directory_req_queue_list->count(replaced_address) == 1);
   processNullifyReq(nullify_req);

   return directory_entry;
}

void
DramDirectoryCntlr::processUnifiedReqFromL2Cache(ShmemReq* directory_req, DirectoryEntry* directory_entry)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   ShmemMsg::Type directory_req_type = directory_req->getShmemMsg()->getType();

   // Get directory_entry if NOT aready retrieved
   if (!directory_entry)
      directory_entry = _directory->getDirectoryEntry(address);
   
   // Get Mode
   Classifier* classifier = dynamic_cast<AdaptiveDirectoryEntry*>(directory_entry)->getClassifier();
   Mode mode = classifier->getMode(requester, directory_req_type, directory_entry);

   switch (mode)
   {
   case PRIVATE_MODE:
   case PRIVATE_SHARER_MODE:
      processPrivateReqFromL2Cache(directory_req, directory_entry);
      break;

   case REMOTE_MODE:
   case REMOTE_SHARER_MODE:
      {
         bool completed = transitionToRemoteMode(directory_req, directory_entry, mode);
         if (completed)
            processRemoteAccessReqFromL2Cache(directory_req, directory_entry);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized cache line mode(%u)", mode);
      break;
   }
}

void
DramDirectoryCntlr::processWriteUnlockReqFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();
   IntPtr address = shmem_msg->getAddress();
   ShmemReq* directory_req = new ShmemReq(shmem_msg, msg_time);

   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   processRemoteAccessReqFromL2Cache(directory_req, directory_entry);
}

bool
DramDirectoryCntlr::transitionToRemoteMode(ShmemReq* directory_req, DirectoryEntry* directory_entry, Mode mode)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   bool modeled = directory_req->getShmemMsg()->isModeled();
   ShmemMsg::Type directory_req_type = TYPE(directory_req);
   // Allocate remote copy on current tile now
   tile_id_t remote_location = getTileId();

   switch (directory_req_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      if (mode == REMOTE_MODE)
         return allocateExclusiveCacheLine(address, remote_location, REMOTE_MODE, directory_entry, requester, modeled);
      else // (mode == REMOTE_SHARER_MODE)
         return allocateSharedCacheLine(address, remote_location, REMOTE_SHARER_MODE, directory_entry, requester, modeled);

   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      return allocateExclusiveCacheLine(address, remote_location, mode, directory_entry, requester, modeled);
   
   default:
      LOG_PRINT_ERROR("Unrecognized request from L2 cache(%u)", directory_req_type);
      return false;      
   }
}

void
DramDirectoryCntlr::processRemoteAccessReqFromL2Cache(ShmemReq* directory_req, DirectoryEntry* directory_entry)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   bool modeled = directory_req->getShmemMsg()->isModeled();
   ShmemMsg::Type directory_req_type = TYPE(directory_req);

   tile_id_t remote_location = getTileId();
   ShmemMsg::Type remote_req_type = ShmemMsg::INVALID;

   switch (directory_req_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      remote_req_type = ShmemMsg::REMOTE_READ_REQ;
      break;
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
      remote_req_type = ShmemMsg::REMOTE_READ_LOCK_REQ;
      break;
   case ShmemMsg::UNIFIED_WRITE_REQ:
      remote_req_type = ShmemMsg::REMOTE_WRITE_REQ;
      break;
   case ShmemMsg::WRITE_UNLOCK_REQ:
      remote_req_type = ShmemMsg::REMOTE_WRITE_UNLOCK_REQ;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized Req Type(%u)", directory_req_type);
      break;
   }

   ShmemMsg remote_req(remote_req_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                       address,
                       requester, modeled);
   getMemoryManager()->sendMsg(remote_location, remote_req);
}

void
DramDirectoryCntlr::processRemoteAccessReplyFromL2Cache(tile_id_t sender, ShmemMsg* remote_reply)
{
   IntPtr address = remote_reply->getAddress();
   tile_id_t requester = remote_reply->getRequester();
   bool modeled = remote_reply->isModeled();

   ShmemMsg::Type remote_reply_type = remote_reply->getType();
   ShmemMsg::Type directory_reply_type = ShmemMsg::INVALID;

   switch (remote_reply_type)
   {
   case ShmemMsg::REMOTE_READ_REPLY:
      directory_reply_type = ShmemMsg::READ_REPLY;
      break;
   case ShmemMsg::REMOTE_READ_LOCK_REPLY:
      directory_reply_type = ShmemMsg::READ_LOCK_REPLY;
      break;
   case ShmemMsg::REMOTE_WRITE_REPLY:
      directory_reply_type = ShmemMsg::WRITE_REPLY;
      break;
   case ShmemMsg::REMOTE_WRITE_UNLOCK_REPLY:
      directory_reply_type = ShmemMsg::WRITE_UNLOCK_REPLY;
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized remote reply type(%u)", remote_reply_type);
      break;
   }

   ShmemMsg l2_reply(directory_reply_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                     address,
                     remote_reply->getDataBuf(), remote_reply->getDataBufSize(),
                     requester, modeled);
   getMemoryManager()->sendMsg(requester, l2_reply);

   // Process the next request from L2 cache
   processNextReqFromL2Cache(address);
}

void
DramDirectoryCntlr::processNullifyReq(ShmemReq* directory_req)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   bool modeled = directory_req->getShmemMsg()->isModeled();
   
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   bool completed = nullifyCacheLine(address, directory_entry, requester, modeled);
   if (completed)
      processNextReqFromL2Cache(address);
}

void
DramDirectoryCntlr::processPrivateReqFromL2Cache(ShmemReq* directory_req, DirectoryEntry* directory_entry)
{
   ShmemMsg::Type directory_req_type = TYPE(directory_req);
   switch (directory_req_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      processShReqFromL2Cache(directory_req, directory_entry);
      break;
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      processExReqFromL2Cache(directory_req, directory_entry);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized req from L2 cache(%u)", directory_req_type);
      break;
   }
   
}

void
DramDirectoryCntlr::processShReqFromL2Cache(ShmemReq* directory_req, DirectoryEntry* directory_entry)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   bool modeled = directory_req->getShmemMsg()->isModeled();
   
   bool completed = allocateSharedCacheLine(address, requester, PRIVATE_MODE, directory_entry, requester, modeled);
   if (completed)
      processNextReqFromL2Cache(address);
}

void
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* directory_req, DirectoryEntry* directory_entry)
{
   IntPtr address = directory_req->getShmemMsg()->getAddress();
   tile_id_t requester = directory_req->getShmemMsg()->getRequester();
   bool modeled = directory_req->getShmemMsg()->isModeled();
   
   bool completed = allocateExclusiveCacheLine(address, requester, PRIVATE_MODE, directory_entry, requester, modeled);
   if (completed)
      processNextReqFromL2Cache(address);
}

bool
DramDirectoryCntlr::nullifyCacheLine(IntPtr address, DirectoryEntry* directory_entry, tile_id_t requester, bool modeled)
{
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                            address, requester, modeled);
         getMemoryManager()->sendMsg(directory_entry->getOwner(), shmem_msg);
         return false;
      }

   case DirectoryState::OWNED:
      {
         LOG_ASSERT_ERROR(directory_entry->getOwner() != INVALID_TILE_ID,
                          "Address(%#lx), State(OWNED), owner(%i)", address, directory_entry->getOwner());

         // FLUSH_REQ to Owner
         // INV_REQ to all sharers except owner (Also sent to the Owner for sake of convenience)
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendShmemMsg(ShmemMsg::INV_FLUSH_COMBINED_REQ, address,
                      directory_entry->getOwner(), all_tiles_sharers, sharers_list,
                      requester, modeled);
         return false;
      }

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
                          "Address(%#lx), State(SHARED), owner(%i)", address, directory_entry->getOwner());
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendShmemMsg(ShmemMsg::INV_REQ, address, 
                      INVALID_TILE_ID, all_tiles_sharers, sharers_list,
                      requester, modeled);
         return false;
      }

   case DirectoryState::UNCACHED:
      {
         _directory->invalidateDirectoryEntry(address);
         return true;
      }

   default:
      LOG_PRINT_ERROR("Unsupported Directory State(%u)", curr_dstate);
      return false;
   }
}

bool
DramDirectoryCntlr::allocateExclusiveCacheLine(IntPtr address, tile_id_t cached_location, Mode mode,
                                               DirectoryEntry* directory_entry, tile_id_t requester, bool modeled)
{
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         if (directory_entry->getOwner() == cached_location)
         {
            return true;
         }
         else
         {
            // FLUSH_REQ to Owner
            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(directory_entry->getOwner(), shmem_msg);
            return false;
         }
      }

   case DirectoryState::OWNED:
      {
         if ((directory_entry->getOwner() == cached_location) && (directory_entry->getNumSharers() == 1))
         {
            directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::MODIFIED);

            ShmemMsg::Type l2_reply_type = (mode == PRIVATE_MODE) ? ShmemMsg::ASYNC_UPGRADE_REPLY : ShmemMsg::UPGRADE_REPLY;
            ShmemMsg shmem_msg(l2_reply_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(cached_location, shmem_msg);
             
            return true; 
         }
         else
         {
            // FLUSH_REQ to Owner
            // INV_REQ to all sharers except owner (Also sent to the owner for sake of convenience)
            
            vector<tile_id_t> sharers_list;
            bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
            
            sendShmemMsg(ShmemMsg::INV_FLUSH_COMBINED_REQ, address, 
                         directory_entry->getOwner(), all_tiles_sharers, sharers_list,
                         requester, modeled);

            return false;
         }
      }

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, 
                          "Address(%#lx), Directory State(SHARED), Num Sharers(%u)",
                          address, directory_entry->getNumSharers());
         
         if ((directory_entry->hasSharer(cached_location)) && (directory_entry->getNumSharers() == 1))
         {
            directory_entry->setOwner(cached_location);
            directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::MODIFIED);

            ShmemMsg::Type l2_reply_type = (mode == PRIVATE_MODE) ? ShmemMsg::ASYNC_UPGRADE_REPLY : ShmemMsg::UPGRADE_REPLY;
            ShmemMsg shmem_msg(l2_reply_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(cached_location, shmem_msg);
            
            return true; 
         }
         else
         {
            // getOneSharer() is a deterministic function
            // FLUSH_REQ to One Sharer (If present)
            // INV_REQ to all other sharers
            
            vector<tile_id_t> sharers_list;
            bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
            
            sendShmemMsg(ShmemMsg::INV_FLUSH_COMBINED_REQ, address,
                         directory_entry->getOneSharer(), all_tiles_sharers, sharers_list,
                         requester, modeled);

            return false;
         }
      }

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() == 0,
                          "Address(%#lx), State(UNCACHED), Num Sharers(%u)",
                          address, directory_entry->getNumSharers());

         bool add_result = addSharer(directory_entry, cached_location);
         LOG_ASSERT_ERROR(add_result, "Address(%#lx), State(UNCACHED)", address);
         
         directory_entry->setOwner(cached_location);
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::MODIFIED);

         ShmemMsg::Type l2_reply_type = (mode == PRIVATE_MODE) ? ShmemMsg::ASYNC_EX_REPLY : ShmemMsg::EX_REPLY;
         return retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
      }

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      return false;
   }
}

bool
DramDirectoryCntlr::allocateSharedCacheLine(IntPtr address, tile_id_t cached_location, Mode mode,
                                            DirectoryEntry* directory_entry, tile_id_t requester, bool modeled)
{
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   ShmemReq* directory_req = _directory_req_queue_list->front(address);

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         if (directory_entry->getOwner() == cached_location)
         {
            assert((mode == REMOTE_MODE) || (mode == REMOTE_SHARER_MODE));
            return true;
         }
         else
         {
            ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(directory_entry->getOwner(), shmem_msg);

            directory_req->setExpectedMsgSender(directory_entry->getOwner());
            return false;
         }
      }

   case DirectoryState::OWNED:
   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, "Address(%#lx), State(%u), Num Sharers(%u)",
                          address, curr_dstate, directory_entry->getNumSharers());

         if (directory_entry->hasSharer(cached_location))
         {
            assert((mode == REMOTE_MODE) || (mode == REMOTE_SHARER_MODE));
            return true;
         }

         bool added = addSharer(directory_entry, cached_location);
         if (!added)
         {
            // Invalidate one sharer
            tile_id_t sharer_id = directory_entry->getOneSharer();

            LOG_ASSERT_ERROR(sharer_id != INVALID_TILE_ID, "Address(%#lx), SH_REQ, state(%u), sharer(%i)",
                             address, curr_dstate, sharer_id);

            // Send a message to the sharer to write back the data and also to invalidate itself
            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(sharer_id, shmem_msg);

            directory_req->setExpectedMsgSender(sharer_id);
            return false;
         }
         else
         {
            // Fetch data from one sharer
            tile_id_t sharer_id = directory_entry->getOneSharer();

            if ( (!directory_req->lookupData()) && (sharer_id != INVALID_TILE_ID) )
            {
               // Remove the added sharer since the request has not been completed
               removeSharer(directory_entry, cached_location, false);

               // Get data from one of the sharers
               ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                                  address, requester, modeled);
               getMemoryManager()->sendMsg(sharer_id, shmem_msg);

               directory_req->setExpectedMsgSender(sharer_id);
               return false;
            }
            else
            {
               ShmemMsg::Type l2_reply_type = (mode == PRIVATE_MODE) ? ShmemMsg::SH_REPLY : ShmemMsg::ASYNC_SH_REPLY;
               return retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
            }
         }
      }

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         bool add_result = addSharer(directory_entry, cached_location);
         LOG_ASSERT_ERROR(add_result, "Address(%#lx), Requester(%i), State(UNCACHED), Num Sharers(%u)",
                          address, requester, directory_entry->getNumSharers());
         
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::SHARED);

         ShmemMsg::Type l2_reply_type = (mode == PRIVATE_MODE) ? ShmemMsg::SH_REPLY : ShmemMsg::ASYNC_SH_REPLY;
         return retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      return false;
   }
}

void
DramDirectoryCntlr::sendShmemMsg(ShmemMsg::Type invalidate_req_type, IntPtr address,
                                 tile_id_t single_receiver, bool all_tiles_sharers, vector<tile_id_t>& sharers_list,
                                 tile_id_t requester, bool modeled)
{
   bool broadcast_inv_req = false;

   if (all_tiles_sharers)
   {
      broadcast_inv_req = true;
      bool reply_expected = (_directory_type == LIMITED_BROADCAST);

      // Broadcast Invalidation Request to all tiles 
      // (irrespective of whether they are sharers or not)
      ShmemMsg shmem_msg(invalidate_req_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE, 
                         address,
                         single_receiver, reply_expected,
                         requester, modeled);
      getMemoryManager()->broadcastMsg(shmem_msg);
   }
   else
   {
      // Send Invalidation Request to only a specific set of sharers
      for (UInt32 i = 0; i < sharers_list.size(); i++)
      {
         ShmemMsg shmem_msg(invalidate_req_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                            address,
                            single_receiver, false,
                            requester, modeled);
         getMemoryManager()->sendMsg(sharers_list[i], shmem_msg);
      }
   }
}

bool
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(IntPtr address, tile_id_t cached_location,
                                                 ShmemMsg::Type reply_type, tile_id_t requester, bool modeled)
{
   // Get data_buf
   ShmemReq* directory_req = _directory_req_queue_list->front(address);
   Byte* data_buf = directory_req->lookupData();

   if (data_buf)
   {
      // I already have the data I need cached
      ShmemMsg directory_reply(reply_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address,
                               data_buf, getCacheLineSize(),
                               requester, modeled);
      getMemoryManager()->sendMsg(cached_location, directory_reply);
      // Delete the cached data
      directory_req->eraseData();
      return true;
   }
   else // (data_buf == NULL)
   {
      // Get the data from DRAM
      fetchDataFromDram(address, requester, modeled);
      return false;
   }
}

void
DramDirectoryCntlr::processInvReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
  
   switch (curr_dstate)
   {
   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR((sender != directory_entry->getOwner()) && (directory_entry->getNumSharers() > 0),
                       "Address(%#lx), State(OWNED), num sharers(%u), sender(%i), owner(%i)",
                       address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
                       "Address(%#lx), State(SHARED), num sharers(%u), sender(%i), owner(%i)",
                       address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      removeSharer(directory_entry, sender, shmem_msg->isReplyExpected());
      if (directory_entry->getNumSharers() == 0)
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      break;

   case DirectoryState::MODIFIED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), INV_REP, State(%u), num sharers(%u), owner(%i)",
            address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (!_directory_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      ShmemReq* directory_req = _directory_req_queue_list->front(address);
      DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
      restartDirectoryReq(directory_req, dstate, sender, shmem_msg);
   }
}

void
DramDirectoryCntlr::processFlushReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
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

   if (!_directory_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      ShmemReq* directory_req = _directory_req_queue_list->front(address);
      // First save the data in the buffers in the directory cntlr
      directory_req->insertData(shmem_msg->getDataBuf(), getCacheLineSize());
      
      // Write-back to memory in certain circumstances
      switch (TYPE(directory_req))
      {
      case ShmemMsg::UNIFIED_READ_REQ:
         {
            DirectoryState::Type initial_dstate = curr_dstate;
            DirectoryState::Type final_dstate = directory_block_info->getDState();

            if ((initial_dstate == DirectoryState::MODIFIED || initial_dstate == DirectoryState::OWNED)
               && (final_dstate == DirectoryState::SHARED || final_dstate == DirectoryState::UNCACHED))
            {
               storeDataInDram(address, shmem_msg->getDataBuf(), shmem_msg->getRequester(), shmem_msg->isModeled());
            }
         }
         break;

      case ShmemMsg::UNIFIED_READ_LOCK_REQ:
      case ShmemMsg::UNIFIED_WRITE_REQ:
         break;

      case ShmemMsg::NULLIFY_REQ:
         storeDataInDram(address, shmem_msg->getDataBuf(), shmem_msg->getRequester(), shmem_msg->isModeled());
         break;

      default:
         LOG_PRINT_ERROR("Unrecognzied directory req type(%u)", TYPE(directory_req));
         break;
      }

      DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
      restartDirectoryReq(directory_req, dstate, sender, shmem_msg);
   }
   else
   {
      // This was just an eviction
      // Write Data to Dram
      storeDataInDram(address, shmem_msg->getDataBuf(), shmem_msg->getRequester(), shmem_msg->isModeled());
   }
}

void
DramDirectoryCntlr::processWbReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   assert(directory_entry);
   
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();

   assert(! shmem_msg->isReplyExpected());

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                       "Address(%#lx), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
      LOG_ASSERT_ERROR(_directory_req_queue_list->count(address) > 0,
                       "Address(%#lx), WB_REP, req queue empty!!", address);

      directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::OWNED);
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
      LOG_PRINT_ERROR("Address(%#lx), WB_REP, State(%u), num sharers(%u), owner(%i)",
                      address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (!_directory_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      ShmemReq* directory_req = _directory_req_queue_list->front(address);
      // First save the data in one of the buffers in the directory cntlr
      directory_req->insertData(shmem_msg->getDataBuf(), getCacheLineSize());

      DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
      restartDirectoryReq(directory_req, dstate, sender, shmem_msg);
   }
   else
   {
      // Ignore the data since this data is already present in the caches of some tiles
      LOG_PRINT_ERROR("Address(%#lx), WB_REP, NO requester", address);
   }
}

// Reply from DRAM_CNTLR with the cache line
void
DramDirectoryCntlr::processDramFetchReply(tile_id_t sender, ShmemMsg* dram_reply)
{
   IntPtr address = dram_reply->getAddress();
   LOG_ASSERT_ERROR(!_directory_req_queue_list->empty(address),
                    "Req queue for address(%#lx) empty", address);
   ShmemReq* directory_req = _directory_req_queue_list->front(address);
   directory_req->insertData(dram_reply->getDataBuf(), getCacheLineSize());
  
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address); 
   DirectoryState::Type dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   restartDirectoryReq(directory_req, dstate, sender, dram_reply);
}

// Send a message to the memory controller requesting to fetch data from DRAM
void
DramDirectoryCntlr::fetchDataFromDram(IntPtr address, tile_id_t requester, bool modeled)
{
   ShmemMsg fetch_msg(ShmemMsg::DRAM_FETCH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::DRAM_CNTLR,
                      address, requester, modeled);
   getMemoryManager()->sendMsg(getDramHome(address), fetch_msg);
}

// Send a message to the memory controller requesting to store data to DRAM
void
DramDirectoryCntlr::storeDataInDram(IntPtr address, Byte* data_buf, tile_id_t requester, bool modeled)
{
   ShmemMsg store_msg(ShmemMsg::DRAM_STORE_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::DRAM_CNTLR,
                     address,
                     data_buf, getCacheLineSize(),
                     requester, modeled);
   getMemoryManager()->sendMsg(getDramHome(address), store_msg);
}

// Get the location of the memory controller corresponding to a specific address
tile_id_t
DramDirectoryCntlr::getDramHome(IntPtr address)
{
   return _dram_home_lookup->getHome(address);
}

void 
DramDirectoryCntlr::restartDirectoryReq(ShmemReq* directory_req, DirectoryState::Type dstate,
                                        tile_id_t last_msg_sender, ShmemMsg* last_msg)
{
   // Update Request & ShmemPerfModel cycle counts
   directory_req->updateTime(getShmemPerfModel()->getCycleCount());
   getShmemPerfModel()->updateCycleCount(directory_req->getTime());

   if (directory_req->restartable(dstate, last_msg_sender, last_msg))
   {
      ShmemMsg::Type directory_req_type = TYPE(directory_req);
      switch (directory_req_type)
      {
      case ShmemMsg::NULLIFY_REQ:
         processNullifyReq(directory_req);
         break;
      default:
         processUnifiedReqFromL2Cache(directory_req);
         break;
      }
   }
}

bool
DramDirectoryCntlr::addSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id)
{
   _directory->getDirectory()->updateSharerStats(directory_entry->getNumSharers(), directory_entry->getNumSharers() + 1);
   return directory_entry->addSharer(sharer_id);
}

void
DramDirectoryCntlr::removeSharer(DirectoryEntry* directory_entry, tile_id_t sharer_id, bool reply_expected)
{
   _directory->getDirectory()->updateSharerStats(directory_entry->getNumSharers(), directory_entry->getNumSharers() - 1);
   directory_entry->removeSharer(sharer_id, reply_expected);
}

UInt32
DramDirectoryCntlr::getCacheLineSize()
{
   return _memory_manager->getCacheLineSize();
}

tile_id_t
DramDirectoryCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
}

ShmemPerfModel*
DramDirectoryCntlr::getShmemPerfModel()
{
   return _memory_manager->getShmemPerfModel();
}

}
