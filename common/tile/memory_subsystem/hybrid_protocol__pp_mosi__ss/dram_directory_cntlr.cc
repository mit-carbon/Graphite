#include "dram_directory_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace HybridProtocol_PPMOSI_SS
{

#define REMOTE(x)          ((x == Mode::REMOTE_LINE)  || (x == Mode::REMOTE_SHARER) )
#define PRIVATE(x)         (x == Mode::PRIVATE)
#define TYPE(x)            (x->getShmemMsg()->getType())

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

   _buffered_req_queue_list = new HashMapQueue<IntPtr,BufferedReq*>();

   _directory_type = DirectoryEntry::parseDirectoryType(directory_type);
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete _buffered_req_queue_list;
   delete _directory;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   if (!IS_BLOCKING_REQ(shmem_msg_type))
   {
      // Update mode information
      DirectoryEntry* directory_entry = _directory->getDirectoryEntry(shmem_msg->getAddress());
      assert(directory_entry);
      Classifier* classifier = dynamic_cast<AdaptiveDirectoryEntry*>(directory_entry)->getClassifier();
      classifier->updateMode(sender, shmem_msg, directory_entry);
   }

   switch (shmem_msg_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      {
         // Add request onto a queue
         IntPtr address = shmem_msg->getAddress();
         UInt64 msg_time = getShmemPerfModel()->getCycleCount();
         BufferedReq* buffered_req = new BufferedReq(shmem_msg, msg_time);
         _buffered_req_queue_list->enqueue(address, buffered_req);
         LOG_PRINT("Address(%#lx), Requester(%i), ReqType(%s), Count(%u) enqueued",
                   address, sender, SPELL_SHMSG(shmem_msg->getType()),
                   (UInt32) _buffered_req_queue_list->count(address));

         if (_buffered_req_queue_list->count(address) == 1)
         {
            processUnifiedReqFromL2Cache(buffered_req);
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

   assert(_buffered_req_queue_list->count(address) >= 1);
   BufferedReq* completed_buffered_req = _buffered_req_queue_list->dequeue(address);
   
   ShmemMsg* completed_directory_req = completed_buffered_req->getShmemMsg();
   LOG_PRINT("Address(%#lx), Requester(%i), ReqType(%s) dequeued",
             address, completed_directory_req->getRequester(), SPELL_SHMSG(completed_directory_req->getType()));

   delete completed_buffered_req;

   if (!_buffered_req_queue_list->empty(address))
   {
      LOG_PRINT("A new shmem req for address(%#lx) found", address);
      BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
      
      // Update the Shared Mem Cycle Counts
      buffered_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(buffered_req->getTime());

      processUnifiedReqFromL2Cache(buffered_req);
   }

   LOG_PRINT("End processNextReqFromL2Cache(%#lx)", address);
}

DirectoryEntry*
DramDirectoryCntlr::processDirectoryEntryAllocationReq(BufferedReq* buffered_req)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   IntPtr address = directory_req->getAddress();
   tile_id_t requester = directory_req->getRequester();
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
           (_buffered_req_queue_list->count((*it)->getAddress()) == 0)
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
                        replaced_address, requester, msg_modeled);

   BufferedReq* nullify_req = new BufferedReq(&nullify_msg, msg_time);
   _buffered_req_queue_list->enqueue(replaced_address, nullify_req);

   assert(_buffered_req_queue_list->count(replaced_address) == 1);
   processNullifyReq(nullify_req);

   return directory_entry;
}

void
DramDirectoryCntlr::processUnifiedReqFromL2Cache(BufferedReq* buffered_req)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();
   IntPtr address = directory_req->getAddress();
   tile_id_t requester = directory_req->getRequester();
   
   // Get directory_entry
   // If cannot find an entry, allocate a new entry by replacing existing one
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   if (!directory_entry)
      directory_entry = processDirectoryEntryAllocationReq(buffered_req);
   
   // Update classifier state
   Classifier* classifier = dynamic_cast<AdaptiveDirectoryEntry*>(directory_entry)->getClassifier();
   classifier->updateMode(requester, directory_req, directory_entry);

   processDirectoryReq(buffered_req, directory_entry);
}

void
DramDirectoryCntlr::processDirectoryReq(BufferedReq* buffered_req, DirectoryEntry* directory_entry)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();
   tile_id_t requester = directory_req->getRequester();
   
   // Get Mode
   Classifier* classifier = dynamic_cast<AdaptiveDirectoryEntry*>(directory_entry)->getClassifier();
   Mode::Type mode = (requester == getTileID()) ? Mode::PRIVATE : (classifier->getMode(requester));
   LOG_PRINT("Address(%#lx), Requester(%i), ReqType(%s), Mode(%s), DState(%s) process",
             directory_req->getAddress(), requester, SPELL_SHMSG(directory_req->getType()), SPELL_MODE(mode),
             SPELL_DSTATE(directory_entry->getDirectoryBlockInfo()->getDState()));
   buffered_req->setMode(mode);

   switch (mode)
   {
   case Mode::PRIVATE:
      processPrivateReqFromL2Cache(buffered_req, directory_entry);
      break;

   case Mode::REMOTE_LINE:
   case Mode::REMOTE_SHARER:
      {
         bool completed = transitionToRemoteMode(buffered_req, directory_entry, mode);
         if (completed)
            processRemoteAccessReqFromL2Cache(buffered_req, directory_entry);
      }
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized cache line mode(%u)", mode);
      break;
   }
}

void
DramDirectoryCntlr::processWriteUnlockReqFromL2Cache(tile_id_t sender, ShmemMsg* directory_req)
{
   IntPtr address = directory_req->getAddress();
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   LOG_ASSERT_ERROR(directory_entry, "Cant find directory entry for address(%#lx)", address);
   LOG_PRINT("Address(%#lx), Requester(%i), ReqType(WRITE_UNLOCK_REQ), Mode(REMOTE_LINE/SHARER), DState(%s) process",
             address, sender, SPELL_DSTATE(directory_entry->getDirectoryBlockInfo()->getDState()));

   BufferedReq* buffered_req = new BufferedReq(directory_req, getShmemPerfModel()->getCycleCount());
   processRemoteAccessReqFromL2Cache(buffered_req, directory_entry);
   delete buffered_req;
}

bool
DramDirectoryCntlr::transitionToRemoteMode(BufferedReq* buffered_req, DirectoryEntry* directory_entry, Mode::Type mode)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   IntPtr address = directory_req->getAddress();
   tile_id_t requester = directory_req->getRequester();
   bool modeled = directory_req->isModeled();
   ShmemMsg::Type directory_req_type = directory_req->getType();

   // Allocate remote copy on current tile now
   tile_id_t remote_location = getTileID();

   switch (directory_req_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      if (mode == Mode::REMOTE_LINE)
         return allocateExclusiveCacheLine(address, directory_req_type, remote_location, Mode::REMOTE_LINE, directory_entry, requester, modeled);
      else // (mode == Mode::REMOTE_SHARER)
         return allocateSharedCacheLine(address, directory_req_type, remote_location, Mode::REMOTE_SHARER, directory_entry, requester, modeled);

   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      return allocateExclusiveCacheLine(address, directory_req_type, remote_location, mode, directory_entry, requester, modeled);
   
   default:
      LOG_PRINT_ERROR("Unrecognized request from L2 cache(%u)", directory_req_type);
      return false;      
   }
}

void
DramDirectoryCntlr::processRemoteAccessReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   ShmemMsg::Type directory_req_type = directory_req->getType();
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

   tile_id_t remote_location = getTileID();
   buffered_req->setExpectedMsgSender(remote_location);
   
   ShmemMsg remote_req(remote_req_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                       directory_req->getAddress(),
                       directory_req->getOffset(), directory_req->getDataLength(),
                       directory_req->getDataBuf(), directory_req->getDataBufSize(),
                       directory_req->getRequester(), directory_req->isModeled());
   getMemoryManager()->sendMsg(remote_location, remote_req);
}

void
DramDirectoryCntlr::processRemoteAccessReplyFromL2Cache(tile_id_t sender, ShmemMsg* remote_reply)
{
   IntPtr address = remote_reply->getAddress();
   tile_id_t requester = remote_reply->getRequester();

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
                     requester, remote_reply->isModeled());
   getMemoryManager()->sendMsg(requester, l2_reply);

   if (remote_reply_type != ShmemMsg::REMOTE_READ_LOCK_REPLY)
   {
      // Process the next request from L2 cache
      processNextReqFromL2Cache(address);
   }
}

void
DramDirectoryCntlr::processNullifyReq(BufferedReq* buffered_req)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   IntPtr address = directory_req->getAddress();
   tile_id_t requester = directory_req->getRequester();
   bool modeled = directory_req->isModeled();
   
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   LOG_ASSERT_ERROR(directory_entry, "Could not find directory_entry for address(%#lx)", address);
   bool completed = nullifyCacheLine(address, directory_entry, requester, modeled);
   if (completed)
      processNextReqFromL2Cache(address);
}

void
DramDirectoryCntlr::processPrivateReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry)
{
   ShmemMsg::Type directory_req_type = TYPE(buffered_req);
   switch (directory_req_type)
   {
   case ShmemMsg::UNIFIED_READ_REQ:
      processShReqFromL2Cache(buffered_req, directory_entry);
      break;
   case ShmemMsg::UNIFIED_READ_LOCK_REQ:
   case ShmemMsg::UNIFIED_WRITE_REQ:
      processExReqFromL2Cache(buffered_req, directory_entry);
      break;
   default:
      LOG_PRINT_ERROR("Unrecognized req from L2 cache(%u)", directory_req_type);
      break;
   }
   
}

void
DramDirectoryCntlr::processExReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   IntPtr address = directory_req->getAddress();
   ShmemMsg::Type directory_req_type = directory_req->getType();
   tile_id_t requester = directory_req->getRequester();
   bool modeled = directory_req->isModeled();
   
   bool completed = allocateExclusiveCacheLine(address, directory_req_type, requester, Mode::PRIVATE, directory_entry, requester, modeled);
   if (completed)
      processNextReqFromL2Cache(address);
}

void
DramDirectoryCntlr::processShReqFromL2Cache(BufferedReq* buffered_req, DirectoryEntry* directory_entry)
{
   ShmemMsg* directory_req = buffered_req->getShmemMsg();

   IntPtr address = directory_req->getAddress();
   ShmemMsg::Type directory_req_type = directory_req->getType();
   tile_id_t requester = directory_req->getRequester();
   bool modeled = directory_req->isModeled();
   
   bool completed = allocateSharedCacheLine(address, directory_req_type, requester, Mode::PRIVATE, directory_entry, requester, modeled);
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
   case DirectoryState::EXCLUSIVE:
   case DirectoryState::OWNED:
   case DirectoryState::SHARED:
      {
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         
         sendShmemMsg(ShmemMsg::INV_FLUSH_COMBINED_REQ, address,
                      directory_entry->getOneSharer(), all_tiles_sharers, sharers_list,
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
DramDirectoryCntlr::allocateExclusiveCacheLine(IntPtr address, ShmemMsg::Type directory_req_type, tile_id_t cached_location, Mode::Type mode,
                                               DirectoryEntry* directory_entry, tile_id_t requester, bool modeled)
{
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
   LOG_PRINT("allocateExclusiveCacheLine: Address(%#lx), DState(%s)", address, SPELL_DSTATE(curr_dstate));

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
   case DirectoryState::EXCLUSIVE:
      {
         if (directory_entry->getOwner() == cached_location)
         {
            if PRIVATE(mode)
            {
               ShmemMsg shmem_msg(ShmemMsg::READY_REPLY, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                                  address, requester, modeled);
               getMemoryManager()->sendMsg(requester, shmem_msg);
            }
            return true;
         }
         else
         {
            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(directory_entry->getOwner(), shmem_msg);
            return false;
         }
      }

   case DirectoryState::OWNED:
   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, 
                          "Address(%#lx), Directory State(%s), Num Sharers(%u)",
                          address, SPELL_DSTATE(curr_dstate), directory_entry->getNumSharers());
         
         // getOneSharer() is a deterministic function
         // FLUSH_REPLY from Owner / One Sharer
         // INV_REQ from all other sharers
         
         vector<tile_id_t> sharers_list;
         bool all_tiles_sharers = directory_entry->getSharersList(sharers_list);
         tile_id_t flush_sharer = (curr_dstate == DirectoryState::OWNED) ?
                                  directory_entry->getOwner() : directory_entry->getOneSharer();
         
         sendShmemMsg(ShmemMsg::INV_FLUSH_COMBINED_REQ, address,
                      flush_sharer, all_tiles_sharers, sharers_list,
                      requester, modeled);
         return false;
      }

   case DirectoryState::UNCACHED:
      {
         // Modifiy the directory entry contents
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() == 0,
                          "Address(%#lx), State(UNCACHED), Num Sharers(%u)",
                          address, directory_entry->getNumSharers());

         ShmemMsg::Type l2_reply_type = REMOTE(mode) ? ShmemMsg::ASYNC_EX_REPLY : ShmemMsg::EX_REPLY;
         bool data_sent = retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
         if (data_sent)
         {
            bool add_result = directory_entry->addSharer(cached_location);
            LOG_ASSERT_ERROR(add_result, "Address(%#lx), State(UNCACHED)", address);
            
            directory_entry->setOwner(cached_location);

            bool cache_line_dirty = buffered_req->isCacheLineDirty();
            DirectoryState::Type new_dstate = (cache_line_dirty) ? DirectoryState::MODIFIED : DirectoryState::EXCLUSIVE;
            directory_entry->getDirectoryBlockInfo()->setDState(new_dstate);
         }
         return data_sent;
      }

   default:
      LOG_PRINT_ERROR("Unsupported Directory State(%u)", curr_dstate);
      return false;
   }
}

bool
DramDirectoryCntlr::allocateSharedCacheLine(IntPtr address, ShmemMsg::Type directory_req_type, tile_id_t cached_location, Mode::Type mode,
                                            DirectoryEntry* directory_entry, tile_id_t requester, bool modeled)
{
   LOG_ASSERT_ERROR(directory_req_type == ShmemMsg::UNIFIED_READ_REQ, "directory_req_type(%s)", SPELL_SHMSG(directory_req_type));
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   BufferedReq* buffered_req = _buffered_req_queue_list->front(address);

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
   case DirectoryState::EXCLUSIVE:
      {
         if ((directory_entry->getOwner() == cached_location))
         {
            if PRIVATE(mode)
            {
               ShmemMsg shmem_msg(ShmemMsg::READY_REPLY, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                                  address, requester, modeled);
               getMemoryManager()->sendMsg(requester, shmem_msg);
            }
            return true;
         }
         else
         {
            ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(directory_entry->getOwner(), shmem_msg);

            buffered_req->setExpectedMsgSender(directory_entry->getOwner());
            return false;
         }
      }

   case DirectoryState::OWNED:
   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0, "Address(%#lx), State(%s), Num Sharers(%u)",
                          address, SPELL_DSTATE(curr_dstate), directory_entry->getNumSharers());

         if (directory_entry->hasSharer(cached_location))
         {
            if PRIVATE(mode)
            {
               ShmemMsg shmem_msg(ShmemMsg::READY_REPLY, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                                  address, requester, modeled);
               getMemoryManager()->sendMsg(requester, shmem_msg);
            }
            return true;
         }

         tile_id_t sharer_id = (curr_dstate == DirectoryState::OWNED) ?
                               directory_entry->getOwner() : directory_entry->getOneSharer();

         bool added = directory_entry->addSharer(cached_location);
         if (!added)  // Sharer is NOT added
         {
            // Invalidate one sharer
            LOG_ASSERT_ERROR(sharer_id != INVALID_TILE_ID, "Address(%#lx), State(%s), Sharer(%i)",
                             address, SPELL_DSTATE(curr_dstate), sharer_id);

            // Send a message to the sharer to write back the data and also to invalidate itself
            ShmemMsg shmem_msg(ShmemMsg::FLUSH_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address, requester, modeled);
            getMemoryManager()->sendMsg(sharer_id, shmem_msg);

            buffered_req->setExpectedMsgSender(sharer_id);
            return false;
         }
         else  // Sharer is addded
         {
            if (!buffered_req->lookupData()) // Data NOT present
            {
               // Remove added sharer
               directory_entry->removeSharer(cached_location, false);

               // Get data from one of the sharers
               LOG_ASSERT_ERROR(sharer_id != INVALID_TILE_ID, "State(%s), Sharer = INVALID",
                                SPELL_DSTATE(curr_dstate));
               
               ShmemMsg shmem_msg(ShmemMsg::WB_REQ, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                                  address, requester, modeled);
               getMemoryManager()->sendMsg(sharer_id, shmem_msg);

               buffered_req->setExpectedMsgSender(sharer_id);
               return false;
            }
            else  // Data present
            {
               ShmemMsg::Type l2_reply_type = REMOTE(mode) ? ShmemMsg::ASYNC_SH_REPLY : ShmemMsg::SH_REPLY;
               bool data_sent = retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
               assert(data_sent);
               
               bool cache_line_dirty = buffered_req->isCacheLineDirty();
               if (cache_line_dirty)
               {
                  assert(curr_dstate == DirectoryState::SHARED);
                  // Dirty status is being handed over to the new sharer
                  directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::OWNED);
                  directory_entry->setOwner(cached_location);
               }
               return true;
            }
         }
      }

   case DirectoryState::UNCACHED:
      {
         // The first sharer, so put in EXCLUSIVE/MODIFIED state
         ShmemMsg::Type l2_reply_type = REMOTE(mode) ? ShmemMsg::ASYNC_EX_REPLY : ShmemMsg::EX_REPLY;
         bool data_sent = retrieveDataAndSendToL2Cache(address, cached_location, l2_reply_type, requester, modeled);
         if (data_sent)
         {         
            // Modifiy the directory entry contents
            bool add_result = directory_entry->addSharer(cached_location);
            LOG_ASSERT_ERROR(add_result, "Address(%#lx), Requester(%i), State(UNCACHED), Num Sharers(%u)",
                             address, requester, directory_entry->getNumSharers());
            
            bool cache_line_dirty = buffered_req->isCacheLineDirty();
            DirectoryState::Type new_dstate = cache_line_dirty ? DirectoryState::MODIFIED : DirectoryState::EXCLUSIVE;
            directory_entry->getDirectoryBlockInfo()->setDState(new_dstate);
            directory_entry->setOwner(cached_location);
         }

         return data_sent;
      }

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
   BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
   Byte* data_buf = buffered_req->lookupData();

   if (data_buf)
   {
      bool cache_line_dirty = buffered_req->isCacheLineDirty();
      // I already have the data I need cached
      ShmemMsg directory_reply(reply_type, MemComponent::DRAM_DIRECTORY, MemComponent::L2_CACHE,
                               address,
                               data_buf, getCacheLineSize(),
                               cache_line_dirty,
                               requester, modeled);
      getMemoryManager()->sendMsg(cached_location, directory_reply);

      // Delete the cached data
      buffered_req->eraseData();
      
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
   case DirectoryState::EXCLUSIVE:
      LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                       "Address(%#lx), State(EXCLUSIVE), sender(%i), owner(%i)",
                       address, sender, directory_entry->getOwner());
      assert(!shmem_msg->isReplyExpected());
      directory_entry->removeSharer(sender, false);
      directory_entry->setOwner(INVALID_TILE_ID);
      directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      break;

   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR((sender != directory_entry->getOwner()) && (directory_entry->getNumSharers() > 1),
                       "Address(%#lx), State(OWNED), num sharers(%u), sender(%i), owner(%i)",
                       address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      directory_entry->removeSharer(sender, shmem_msg->isReplyExpected());
      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
                       "Address(%#lx), State(SHARED), num sharers(%u), sender(%i), owner(%i)",
                       address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

      directory_entry->removeSharer(sender, shmem_msg->isReplyExpected());
      if (directory_entry->getNumSharers() == 0)
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      break;

   case DirectoryState::MODIFIED:
   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), INV_REPLY, sender(%i), state(%s), num sharers(%u), owner(%i)",
                      address, sender, SPELL_DSTATE(curr_dstate), directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (!_buffered_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
      restartDirectoryReqIfReady(buffered_req, directory_entry, sender, shmem_msg);
   }
}

void
DramDirectoryCntlr::processFlushReplyFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address);
   assert(directory_entry);
   DirectoryState::Type curr_dstate = directory_entry->getDirectoryBlockInfo()->getDState();
   
   LOG_ASSERT_ERROR(curr_dstate != DirectoryState::UNCACHED,
                    "Address(%#lx), State(%u)", address, curr_dstate);

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         LOG_ASSERT_ERROR(shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(NO)");
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), State(%s), sender(%i), owner(%i)",
                          address, SPELL_DSTATE(curr_dstate), sender, directory_entry->getOwner());
         assert(! shmem_msg->isReplyExpected());
         directory_entry->removeSharer(sender, false);
         directory_entry->setOwner(INVALID_TILE_ID);
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::EXCLUSIVE:
      {
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), State(%s), sender(%i), owner(%i)",
                          address, SPELL_DSTATE(curr_dstate), sender, directory_entry->getOwner());
         assert(! shmem_msg->isReplyExpected());
         directory_entry->removeSharer(sender, false);
         directory_entry->setOwner(INVALID_TILE_ID);
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::OWNED:
      {
         LOG_ASSERT_ERROR(shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(NO)");
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), State(OWNED), owner(%i), num sharers(%u)",
                          address, directory_entry->getOwner(), directory_entry->getNumSharers());
         
         directory_entry->removeSharer(sender, shmem_msg->isReplyExpected());
         directory_entry->setOwner(INVALID_TILE_ID);
         DirectoryState::Type new_dstate = (directory_entry->getNumSharers() > 0) ?
                                           DirectoryState::SHARED : DirectoryState::UNCACHED;
         directory_entry->getDirectoryBlockInfo()->setDState(new_dstate);
      }
      break;

   case DirectoryState::SHARED:
      {
         LOG_ASSERT_ERROR(!shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(YES)");
         LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_TILE_ID) && (directory_entry->getNumSharers() > 0),
                          "Address(%#lx), State(SHARED), owner(%i), num sharers(%u)",
                          address, directory_entry->getOwner(), directory_entry->getNumSharers());

         directory_entry->removeSharer(sender, shmem_msg->isReplyExpected());
         if (directory_entry->getNumSharers() == 0)
            directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::UNCACHED);
      }
      break;

   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), FLUSH_REPLY, sender(%i), state(%s), num sharers(%u), owner(%i)",
                      address, sender, SPELL_DSTATE(curr_dstate), directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (!_buffered_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
      // First save the data in the buffers in the directory cntlr
      buffered_req->insertData(shmem_msg->getDataBuf(), getCacheLineSize());
      buffered_req->setCacheLineDirty(shmem_msg->isCacheLineDirty());
      
      // Write-back to memory for nullify requests
      if (TYPE(buffered_req) == ShmemMsg::NULLIFY_REQ)
      {
         storeDataInDram(address, shmem_msg->getDataBuf(), shmem_msg->getRequester(), shmem_msg->isModeled());
         // Delete the cached data
         buffered_req->eraseData();
      }

      restartDirectoryReqIfReady(buffered_req, directory_entry, sender, shmem_msg);
   }
   else
   {
      assert(shmem_msg->isCacheLineDirty());
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

   assert(!shmem_msg->isReplyExpected());

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      {
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
         LOG_ASSERT_ERROR(_buffered_req_queue_list->count(address) > 0,
                          "Address(%#lx), WB_REP, req queue empty!!", address);

         LOG_ASSERT_ERROR(shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(NO)");
         directory_entry->getDirectoryBlockInfo()->setDState(DirectoryState::OWNED);
      }
      break;

   case DirectoryState::EXCLUSIVE:
      {
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                          "Address(%#lx), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
         LOG_ASSERT_ERROR(_buffered_req_queue_list->count(address) > 0,
                          "Address(%#lx), WB_REP, req queue empty!!", address);

         DirectoryState::Type new_dstate = (shmem_msg->isCacheLineDirty()) ? DirectoryState::OWNED : DirectoryState::SHARED;
         directory_entry->getDirectoryBlockInfo()->setDState(new_dstate);
         if (new_dstate == DirectoryState::SHARED)
            directory_entry->setOwner(INVALID_TILE_ID);
      }
      break;

   case DirectoryState::OWNED:
      LOG_ASSERT_ERROR(shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(NO)");
      LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
                       "Address(%#lx), sender(%i), NOT owner", address, sender);
      break;

   case DirectoryState::SHARED:
      LOG_ASSERT_ERROR(!shmem_msg->isCacheLineDirty(), "Cache-Line-Dirty(YES)");
      LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_TILE_ID,
                       "Address(%#lx), State(%u), Owner(%i)", address, directory_entry->getOwner());
      LOG_ASSERT_ERROR(directory_entry->hasSharer(sender),
                       "Address(%#lx), sender(%i), NOT sharer", address, sender);
      break;

   case DirectoryState::UNCACHED:
   default:
      LOG_PRINT_ERROR("Address(%#lx), WB_REPLY, sender(%i), state(%s), num sharers(%u), owner(%i)",
                      address, sender, SPELL_DSTATE(curr_dstate), directory_entry->getNumSharers(), directory_entry->getOwner());
      break;
   }

   if (!_buffered_req_queue_list->empty(address))
   {
      // Get the latest request for the data
      BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
      // First save the data in one of the buffers in the directory cntlr
      buffered_req->insertData(shmem_msg->getDataBuf(), getCacheLineSize());

      restartDirectoryReqIfReady(buffered_req, directory_entry, sender, shmem_msg);
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
   LOG_ASSERT_ERROR(!_buffered_req_queue_list->empty(address),
                    "Req queue for address(%#lx) empty", address);
   BufferedReq* buffered_req = _buffered_req_queue_list->front(address);
   buffered_req->insertData(dram_reply->getDataBuf(), getCacheLineSize());
  
   DirectoryEntry* directory_entry = _directory->getDirectoryEntry(address); 
   LOG_ASSERT_ERROR(directory_entry, "Could not find directory_entry for address(%#lx)", address);
   restartDirectoryReqIfReady(buffered_req, directory_entry, sender, dram_reply);
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
DramDirectoryCntlr::restartDirectoryReqIfReady(BufferedReq* buffered_req, DirectoryEntry* directory_entry,
                                               tile_id_t last_msg_sender, ShmemMsg* last_msg)
{
   LOG_PRINT("Restart Directory Req: Address(%#lx)", buffered_req->getShmemMsg()->getAddress());
   // Update Buffered Request & ShmemPerfModel cycle counts
   buffered_req->updateTime(getShmemPerfModel()->getCycleCount());
   getShmemPerfModel()->updateCycleCount(buffered_req->getTime());

   if (buffered_req->restartable(directory_entry, last_msg_sender, last_msg))
   {
      ShmemMsg::Type directory_req_type = TYPE(buffered_req);
      switch (directory_req_type)
      {
      case ShmemMsg::NULLIFY_REQ:
         processNullifyReq(buffered_req);
         break;
      default:
         processDirectoryReq(buffered_req, directory_entry);
         break;
      }
   }
}

UInt32
DramDirectoryCntlr::getCacheLineSize()
{
   return _memory_manager->getCacheLineSize();
}

tile_id_t
DramDirectoryCntlr::getTileID()
{
   return _memory_manager->getTile()->getId();
}

ShmemPerfModel*
DramDirectoryCntlr::getShmemPerfModel()
{
   return _memory_manager->getShmemPerfModel();
}

}
