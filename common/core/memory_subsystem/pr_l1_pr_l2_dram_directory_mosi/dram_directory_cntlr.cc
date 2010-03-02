using namespace std;

#include "dram_directory_cntlr.h"
#include "log.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectoryMOSI
{

DramDirectoryCntlr::DramDirectoryCntlr(core_id_t core_id,
      MemoryManager* memory_manager,
      DramCntlr* dram_cntlr,
      UInt32 dram_directory_total_entries,
      UInt32 dram_directory_associativity,
      UInt32 cache_block_size,
      UInt32 dram_directory_max_num_sharers,
      UInt32 dram_directory_max_hw_sharers,
      string dram_directory_type_str,
      UInt32 dram_directory_cache_access_time,
      ShmemPerfModel* shmem_perf_model):
   m_memory_manager(memory_manager),
   m_dram_cntlr(dram_cntlr),
   m_core_id(core_id),
   m_cache_block_size(cache_block_size),
   m_shmem_perf_model(shmem_perf_model)
{
   m_dram_directory_cache = new DramDirectoryCache(
         dram_directory_type_str,
         dram_directory_total_entries,
         dram_directory_associativity,
         cache_block_size,
         dram_directory_max_hw_sharers,
         dram_directory_max_num_sharers,
         dram_directory_cache_access_time,
         m_shmem_perf_model);
   m_dram_directory_req_queue_list = new ReqQueueList();
   m_cached_data_list = new DataList(m_cache_block_size);
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete m_dram_directory_cache;
   delete m_dram_directory_req_queue_list;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::msg_t shmem_msg_type = shmem_msg->getMsgType();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   switch (shmem_msg_type)
   {
      case ShmemMsg::EX_REQ:
      case ShmemMsg::SH_REQ:

         {
            IntPtr address = shmem_msg->getAddress();
            
            // Add request onto a queue
            ShmemReq* shmem_req = new ShmemReq(shmem_msg, msg_time);
            m_dram_directory_req_queue_list->enqueue(address, shmem_req);

            if (m_dram_directory_req_queue_list->size(address) == 1)
            {
               // No data should be cached for this address
               assert(m_cached_data_list->lookup(address) == NULL);
               
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
   LOG_PRINT("Start processNextReqFromL2Cache(0x%x)", address);

   assert(m_dram_directory_req_queue_list->size(address) >= 1);
   ShmemReq* completed_shmem_req = m_dram_directory_req_queue_list->dequeue(address);
   delete completed_shmem_req;

   // No longer should any data be cached for this address
   assert(m_cached_data_list->lookup(address) == NULL);

   if (! m_dram_directory_req_queue_list->empty(address))
   {
      LOG_PRINT("A new shmem req for address(0x%x) found", address);
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update the Shared Mem Cycle Counts appropriately
      getShmemPerfModel()->setCycleCount(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
         processExReqFromL2Cache(shmem_req);
      else if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
         processShReqFromL2Cache(shmem_req);
      else
         LOG_PRINT_ERROR("Unrecognized Request(%u)", shmem_req->getShmemMsg()->getMsgType());
   }
   LOG_PRINT("End processNextReqFromL2Cache(0x%x)", address);
}

DirectoryEntry*
DramDirectoryCntlr::processDirectoryEntryAllocationReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   core_id_t requester = shmem_req->getShmemMsg()->getRequester();
   UInt64 msg_time = getShmemPerfModel()->getCycleCount();

   std::vector<DirectoryEntry*> replacement_candidate_list;
   m_dram_directory_cache->getReplacementCandidates(address, replacement_candidate_list);

   std::vector<DirectoryEntry*>::iterator it;
   std::vector<DirectoryEntry*>::iterator replacement_candidate = replacement_candidate_list.end();
   for (it = replacement_candidate_list.begin(); it != replacement_candidate_list.end(); it++)
   {
      if ( ( (replacement_candidate == replacement_candidate_list.end()) ||
             ((*replacement_candidate)->getNumSharers() > (*it)->getNumSharers()) 
           )
           &&
           (m_dram_directory_req_queue_list->size((*it)->getAddress()) == 0)
         )
      {
         replacement_candidate = it;
      }
   }

   LOG_ASSERT_ERROR(replacement_candidate != replacement_candidate_list.end(),
         "Cant find a directory entry to be replaced with a non-zero request list");

   IntPtr replaced_address = (*replacement_candidate)->getAddress();

   // We get the entry with the lowest number of sharers
   DirectoryEntry* directory_entry = m_dram_directory_cache->replaceDirectoryEntry(replaced_address, address);

   ShmemMsg nullify_msg(ShmemMsg::NULLIFY_REQ, MemComponent::DRAM_DIR, MemComponent::DRAM_DIR, requester, replaced_address, NULL, 0);

   ShmemReq* nullify_req = new ShmemReq(&nullify_msg, msg_time);
   m_dram_directory_req_queue_list->enqueue(replaced_address, nullify_req);

   assert(m_dram_directory_req_queue_list->size(replaced_address) == 1);
   processNullifyReq(nullify_req);

   return directory_entry;
}

void
DramDirectoryCntlr::processNullifyReq(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   core_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         {
            getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ, 
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                  requester /* requester */, 
                  directory_entry->getOwner() /* receiver */, 
                  address);
         }
         break;
  
      case DirectoryState::OWNED:
         {
            LOG_ASSERT_ERROR(directory_entry->getOwner() != INVALID_CORE_ID,
                  "Address(0x%x), State(OWNED), owner(%i)", address, directory_entry->getOwner());

            // FLUSH_REQ to Owner
            getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ,
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                  requester /* requester */,
                  directory_entry->getOwner() /* receiver */,
                  address);

            // INV_REQ to all sharers except owner (Also sent to the Owner for sake of convenience)
            sendInvReq(address, requester, directory_entry->getSharersList());
         }
         break;

      case DirectoryState::SHARED:
         {
            LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_CORE_ID,
                  "Address(0x%x), State(SHARED), owner(%i)", address, directory_entry->getOwner());
            
            sendInvReq(address, requester, directory_entry->getSharersList());
         }
         break;
   
      case DirectoryState::UNCACHED:
         {
            // Send data to Dram
            Byte* cached_data_buf = m_cached_data_list->lookup(shmem_req->getShmemMsg()->getAddress());
            if (cached_data_buf != NULL)
            {
               sendDataToDram(address, shmem_req->getShmemMsg()->getRequester(), cached_data_buf);
               m_cached_data_list->erase(address);
            }
 
            m_dram_directory_cache->invalidateDirectoryEntry(address);
            
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
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   core_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         {
            // FLUSH_REQ to Owner
            getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ, 
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                  requester /* requester */, 
                  directory_entry->getOwner() /* receiver */, 
                  address);
         }
         break;

      case DirectoryState::OWNED:
         {
            if ((directory_entry->getOwner() == requester) && (directory_entry->getNumSharers() == 1))
            {
               directory_block_info->setDState(DirectoryState::MODIFIED);

               getMemoryManager()->sendMsg(ShmemMsg::UPGRADE_REP,
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                     requester /* requester */,
                     requester /* receiver */,
                     address);
               
               processNextReqFromL2Cache(address);
            }
            
            // FLUSH_REQ to Owner
            getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ,
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                  requester /* requester */,
                  directory_entry->getOwner() /* receiver */,
                  address);

            // INV_REQ to all sharers except owner (Also sent to the Owner for sake of convenience)
            sendInvReq(address, requester, directory_entry->getSharersList());
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

               getMemoryManager()->sendMsg(ShmemMsg::UPGRADE_REP,
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                     requester /* requester */,
                     requester /* receiver */,
                     address);
               
               processNextReqFromL2Cache(address);
            }
            else
            {
               // WB_REQ to One Sharer - What if there is no sharer?
               // getOneSharer() is a deterministic function
               if (directory_entry->getOneSharer() != INVALID_CORE_ID)
               {
                  getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ,
                        MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                        requester /* requester */,
                        directory_entry->getOneSharer() /* receiver */,
                        address);
               }

               // INV_REQ to all sharers (Also to the one sharer who was flushed for the sake of convenience)
               sendInvReq(address, requester, directory_entry->getSharersList());
            }
         }
         break;
   
      case DirectoryState::UNCACHED:
         {
            // Modifiy the directory entry contents
            LOG_ASSERT_ERROR(directory_entry->getNumSharers() == 0,
                  "Address(0x%x), State(UNCACHED), Num Sharers(%u)",
                  address, directory_entry->getNumSharers());

            bool add_result = directory_entry->addSharer(requester);
            LOG_ASSERT_ERROR(add_result == true,
                  "Address(0x%x), State(UNCACHED)",
                  address);

            directory_entry->setOwner(requester);
            directory_block_info->setDState(DirectoryState::MODIFIED);

            retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, requester, address);

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
DramDirectoryCntlr::processShReqFromL2Cache(ShmemReq* shmem_req)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   core_id_t requester = shmem_req->getShmemMsg()->getRequester();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   if (directory_entry == NULL)
   {
      directory_entry = processDirectoryEntryAllocationReq(shmem_req);
   }

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         {
            getMemoryManager()->sendMsg(ShmemMsg::WB_REQ, 
                  MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                  requester /* requester */, 
                  directory_entry->getOwner() /* receiver */, 
                  address);

            shmem_req->setResponderCoreId(directory_entry->getOwner());
         }
         break;
   
      case DirectoryState::OWNED:
      case DirectoryState::SHARED:
         {
            LOG_ASSERT_ERROR(directory_entry->getNumSharers() > 0,
                  "Address(0x%x), State(%u), Num Sharers(%u)",
                  address, curr_dstate, directory_entry->getNumSharers());

            // Fetch data from one sharer - Sharer can be the owner too !!
            core_id_t sharer_id = directory_entry->getOneSharer();

            bool add_result = directory_entry->addSharer(requester);
            if (add_result == false)
            {
               LOG_ASSERT_ERROR(sharer_id != INVALID_CORE_ID, "Address(0x%x), SH_REQ, state(%u), sharer(%i)",
                     address, curr_dstate, sharer_id);

               // Send a message to the sharer to write back the data and also to invalidate itself
               getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ,
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                     requester /* requester */,
                     sharer_id /* receiver */,
                     address);

               shmem_req->setResponderCoreId(sharer_id);
            }
            else
            {
               Byte* cached_data_buf = m_cached_data_list->lookup(address);
               if ((cached_data_buf == NULL) && (sharer_id != INVALID_CORE_ID))
               {
                  // Remove the added sharer since the request has not been completed
                  directory_entry->removeSharer(requester);

                  // Get data from one of the sharers
                  getMemoryManager()->sendMsg(ShmemMsg::WB_REQ,
                        MemComponent::DRAM_DIR, MemComponent::L2_CACHE,
                        requester /* requester */,
                        sharer_id /* receiver */,
                        address);

                  shmem_req->setResponderCoreId(sharer_id);
               }
               else
               {
                  retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address);
         
                  // Process Next Request
                  processNextReqFromL2Cache(address);
               }
            }
         }
         break;

      case DirectoryState::UNCACHED:
         {
            // Modifiy the directory entry contents
            bool add_result = directory_entry->addSharer(requester);
            LOG_ASSERT_ERROR(add_result == true,
                  "Address(0x%x), Requester(%i), State(UNCACHED), Num Sharers(%u)",
                  address, requester, directory_entry->getNumSharers());
            
            directory_block_info->setDState(DirectoryState::SHARED);

            retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address);
      
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
DramDirectoryCntlr::sendInvReq(IntPtr address, core_id_t requester, pair<bool, vector<core_id_t> >& sharers_list_pair)
{
   if (sharers_list_pair.first == true)
   {
      // Broadcast Invalidation Request to all cores 
      // (irrespective of whether they are sharers or not)
      getMemoryManager()->broadcastMsg(ShmemMsg::INV_REQ, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            requester /* requester */, 
            address);
   }
   else
   {
      // Send Invalidation Request to only a specific set of sharers
      for (UInt32 i = 0; i < sharers_list_pair.second.size(); i++)
      {
         getMemoryManager()->sendMsg(ShmemMsg::INV_REQ, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               requester /* requester */, 
               sharers_list_pair.second[i] /* receiver */, 
               address);
      }
   }
}

void
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type,
      core_id_t receiver, IntPtr address)
{
   Byte* cached_data_buf = m_cached_data_list->lookup(address);

   if (cached_data_buf != NULL)
   {
      // I already have the data I need cached
      getMemoryManager()->sendMsg(reply_msg_type, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            receiver /* requester */, 
            receiver /* receiver */, 
            address, 
            cached_data_buf, getCacheBlockSize());

      m_cached_data_list->erase(address);
   }
   else
   {
      // Get the data from DRAM
      // This could be directly forwarded to the cache or passed
      // through the Dram Directory Controller

      // I have to get the data from DRAM
      Byte data_buf[getCacheBlockSize()];
      
      m_dram_cntlr->getDataFromDram(address, receiver, data_buf);
      getMemoryManager()->sendMsg(reply_msg_type, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            receiver /* requester */,
            receiver /* receiver */,
            address, 
            data_buf, getCacheBlockSize());
   }
}

void
DramDirectoryCntlr::processInvRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();
  
   switch (curr_dstate)
   {
      case DirectoryState::OWNED:
         LOG_ASSERT_ERROR((sender != directory_entry->getOwner()) && (directory_entry->getNumSharers() > 0),
               "Address(0x%x), State(OWNED), num sharers(%u), sender(%i), owner(%i)",
               address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

         directory_entry->removeSharer(sender);
         break;

      case DirectoryState::SHARED:
         LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_CORE_ID) && (directory_entry->getNumSharers() > 0),
               "Address(0x%x), State(SHARED), num sharers(%u), sender(%i), owner(%i)",
               address, directory_entry->getNumSharers(), sender, directory_entry->getOwner());

         directory_entry->removeSharer(sender);
         if (directory_entry->getNumSharers() == 0)
            directory_block_info->setDState(DirectoryState::UNCACHED);
         break;

      case DirectoryState::MODIFIED:
      case DirectoryState::UNCACHED:
      default:
         LOG_PRINT_ERROR("Address(0x%x), INV_REP, State(%u), num sharers(%u), owner(%i)",
               address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
         break;
   }

   if (m_dram_directory_req_queue_list->size(address) > 0)
   {
      // Get the latest request for the data
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);
      restartShmemReq(sender, shmem_req, directory_block_info->getDState());
   }
}

void
DramDirectoryCntlr::processFlushRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);

   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();
   
   LOG_ASSERT_ERROR(curr_dstate != DirectoryState::UNCACHED,
         "Address(0x%x), State(%u)", address, curr_dstate);

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
               "Address(0x%x), State(MODIFIED), sender(%i), owner(%i)",
               address, sender, directory_entry->getOwner());

         directory_entry->removeSharer(sender);
         directory_entry->setOwner(INVALID_CORE_ID);
         directory_block_info->setDState(DirectoryState::UNCACHED); 
         break;

      case DirectoryState::OWNED:
         LOG_ASSERT_ERROR((directory_entry->getOwner() != INVALID_CORE_ID) && (directory_entry->getNumSharers() > 0),
               "Address(0x%x), State(OWNED), owner(%i), num sharers(%u)",
               address, directory_entry->getOwner(), directory_entry->getNumSharers());
         
         directory_entry->removeSharer(sender);
         if (sender == directory_entry->getOwner())
         {
            directory_entry->setOwner(INVALID_CORE_ID);
            if (directory_entry->getNumSharers() > 0)
               directory_block_info->setDState(DirectoryState::SHARED);
            else
               directory_block_info->setDState(DirectoryState::UNCACHED);
         }
         break;

      case DirectoryState::SHARED:
         LOG_ASSERT_ERROR((directory_entry->getOwner() == INVALID_CORE_ID) && (directory_entry->getNumSharers() > 0),
               "Address(0x%x), State(SHARED), owner(%i), num sharers(%u)",
               address, directory_entry->getOwner(), directory_entry->getNumSharers());

         directory_entry->removeSharer(sender);
         if (directory_entry->getNumSharers() == 0)
            directory_block_info->setDState(DirectoryState::UNCACHED);
         break;

      case DirectoryState::UNCACHED:
      default:
         LOG_PRINT_ERROR("Address(0x%x), FLUSH_REP, State(%u), num sharers(%u), owner(%i)",
               address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
         break;
   }

   if (m_dram_directory_req_queue_list->size(address) > 0)
   {
      // First save the data in one of the buffers in the directory cntlr
      m_cached_data_list->insert(address, shmem_msg->getDataBuf());

      // Get the latest request for the data
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);
      restartShmemReq(sender, shmem_req, directory_block_info->getDState());
   }
   else
   {
      // This was just an eviction
      // Write Data to Dram
      sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());
   }
}

void
DramDirectoryCntlr::processWbRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);
   
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::MODIFIED:
         LOG_ASSERT_ERROR(sender == directory_entry->getOwner(),
               "Address(0x%x), sender(%i), owner(%i)", address, sender, directory_entry->getOwner());
         LOG_ASSERT_ERROR(m_dram_directory_req_queue_list->size(address) > 0,
               "Address(0x%x), WB_REP, req queue empty!!", address);

         directory_block_info->setDState(DirectoryState::OWNED);
         break;

      case DirectoryState::OWNED:
         LOG_ASSERT_ERROR(directory_entry->hasSharer(sender),
               "Address(0x%x), sender(%i), NOT sharer", address, sender);

         break;

      case DirectoryState::SHARED:
         LOG_ASSERT_ERROR(directory_entry->getOwner() == INVALID_CORE_ID,
               "Address(0x%x), State(%u), Owner(%i)", address, directory_entry->getOwner());
         LOG_ASSERT_ERROR(directory_entry->hasSharer(sender),
               "Address(0x%x), sender(%i), NOT sharer", address, sender);

         break;

      case DirectoryState::UNCACHED:
      default:
         LOG_PRINT_ERROR("Address(0x%x), WB_REP, State(%u), num sharers(%u), owner(%i)",
               address, curr_dstate, directory_entry->getNumSharers(), directory_entry->getOwner());
         break;
   }

   if (m_dram_directory_req_queue_list->size(address) > 0)
   {
      // First save the data in one of the buffers in the directory cntlr
      m_cached_data_list->insert(address, shmem_msg->getDataBuf());

      // Get the latest request for the data
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);
      restartShmemReq(sender, shmem_req, directory_block_info->getDState());
   }
   else
   {
      // Ignore the data since this data is already present in the caches of some cores
      LOG_PRINT_ERROR("Address(0x%x), WB_REP, NO requester", address);
   }
}

void 
DramDirectoryCntlr::restartShmemReq(core_id_t sender, ShmemReq* shmem_req, DirectoryState::dstate_t curr_dstate)
{
   // Update Request & ShmemPerfModel times
   shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
   getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

   ShmemMsg::msg_t msg_type = shmem_req->getShmemMsg()->getMsgType();

   switch(msg_type)
   {
      case ShmemMsg::EX_REQ:
         if (curr_dstate == DirectoryState::UNCACHED)
            processExReqFromL2Cache(shmem_req);
         break;

      case ShmemMsg::SH_REQ:
         assert(shmem_req->getResponderCoreId() != INVALID_CORE_ID);
         if (sender == shmem_req->getResponderCoreId())
         {
            shmem_req->setResponderCoreId(INVALID_CORE_ID);
            processShReqFromL2Cache(shmem_req);
         }
         break;

      case ShmemMsg::NULLIFY_REQ:
         if (curr_dstate == DirectoryState::UNCACHED)
            processNullifyReq(shmem_req);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized request type(%u)", msg_type);
         break;
   }
}

void
DramDirectoryCntlr::sendDataToDram(IntPtr address, core_id_t requester, Byte* data_buf)
{
   // Write data to Dram
   m_dram_cntlr->putDataToDram(address, requester, data_buf);
}

DramDirectoryCntlr::DataList::DataList(UInt32 block_size):
   m_block_size(block_size)
{}

DramDirectoryCntlr::DataList::~DataList()
{}

void
DramDirectoryCntlr::DataList::insert(IntPtr address, Byte* data)
{
   Byte* alloc_data = new Byte[m_block_size];
   memcpy(alloc_data, data, m_block_size);

   std::pair<std::map<IntPtr,Byte*>::iterator, bool> ret = m_data_list.insert(std::make_pair<IntPtr,Byte*>(address, alloc_data));
   if (ret.second == false)
   {
      // There is already some data present
      SInt32 equal = memcmp(alloc_data, (ret.first)->second, m_block_size);
      LOG_ASSERT_ERROR(equal == 0, "Address(0x%x), cached data different from now received data");
      delete [] alloc_data;
   }
   
   LOG_ASSERT_ERROR(m_data_list.size() <= Config::getSingleton()->getTotalCores(),
         "m_data_list.size() = %u, m_total_cores = %u",
         m_data_list.size(), Config::getSingleton()->getTotalCores());
}

Byte*
DramDirectoryCntlr::DataList::lookup(IntPtr address)
{
   std::map<IntPtr,Byte*>::iterator data_list_it = m_data_list.find(address);
   if (data_list_it != m_data_list.end())
      return (data_list_it->second);
   else
      return ((Byte*) NULL);
}

void
DramDirectoryCntlr::DataList::erase(IntPtr address)
{
   std::map<IntPtr,Byte*>::iterator data_list_it = m_data_list.find(address);
   LOG_ASSERT_ERROR(data_list_it != m_data_list.end(),
         "Unable to erase address(0x%x) from m_data_list", address);

   delete [] (data_list_it->second);
   m_data_list.erase(data_list_it);
}

}
