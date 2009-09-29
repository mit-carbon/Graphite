using namespace std;

#include "dram_directory_cntlr.h"
#include "log.h"
#include "memory_manager.h"

namespace PrL1PrL2DramDirectory
{

DramDirectoryCntlr::DramDirectoryCntlr(core_id_t core_id,
      MemoryManager* memory_manager,
      DramCntlr* dram_cntlr,
      AddressHomeLookup* dram_home_lookup,
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
   m_dram_home_lookup(dram_home_lookup),
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
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete m_dram_directory_cache;
   delete m_dram_directory_req_queue_list;
}

void
DramDirectoryCntlr::handleMsgFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg, UInt64 msg_time)
{
   ShmemMsg::msg_t shmem_msg_type = shmem_msg->getMsgType();

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
               if (shmem_msg_type == ShmemMsg::EX_REQ)
                  processExReqFromL2Cache(shmem_req);
               else // (shmem_msg_type == ShmemMsg::SH_REQ)
                  processShReqFromL2Cache(shmem_req);
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

   if (! m_dram_directory_req_queue_list->empty(address))
   {
      LOG_PRINT("A new shmem req for address(0x%x) found", address);
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update the Shared Mem Cycle Counts appropriately
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
         processExReqFromL2Cache(shmem_req);
      else
         processShReqFromL2Cache(shmem_req);
   }
   LOG_PRINT("End processNextReqFromL2Cache(0x%x)", address);
}

void
DramDirectoryCntlr::processExReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf)
{
   IntPtr address = shmem_req->getShmemMsg()->getAddress();
   if (m_dram_request_pending[address])
      return;

   core_id_t requester = shmem_req->getShmemMsg()->getRequester();
   
   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();

   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::EXCLUSIVE:
         assert(cached_data_buf == NULL);
         getMemoryManager()->sendMsg(ShmemMsg::FLUSH_REQ, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               requester /* requester */, 
               directory_entry->getOwner() /* receiver */, 
               address);
         break;
   
      case DirectoryState::SHARED:

         {
            assert(cached_data_buf == NULL);
            pair<bool, vector<SInt32> > sharers_list_pair = directory_entry->getSharersList();
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
         break;
   
      case DirectoryState::UNCACHED:

         {
            // Modifiy the directory entry contents
            pair<bool, SInt32> add_result = directory_entry->addSharer(requester);
            assert(add_result.first == true);
            directory_entry->setOwner(requester);
            directory_block_info->setDState(DirectoryState::EXCLUSIVE);

            retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, requester, address, cached_data_buf);
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
   if (m_dram_request_pending[address])
      return;

   core_id_t requester = shmem_req->getShmemMsg()->getRequester();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();

   DirectoryState::dstate_t curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
      case DirectoryState::EXCLUSIVE:
         assert(cached_data_buf == NULL);
         getMemoryManager()->sendMsg(ShmemMsg::WB_REQ, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               requester /* requester */, 
               directory_entry->getOwner() /* receiver */, 
               address);
         break;
   
      case DirectoryState::SHARED:

         {
            pair<bool, SInt32> add_result = directory_entry->addSharer(requester);
            if (add_result.first == false)
            {
               assert(cached_data_buf == NULL);
               // Send a message to another sharer to invalidate that
               getMemoryManager()->sendMsg(ShmemMsg::INV_REQ, 
                     MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
                     requester /* requester */, 
                     add_result.second /* receiver */, 
                     address);
            }
            else
            {
               retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf);
            }
         }
         break;

      case DirectoryState::UNCACHED:

         {
            // Modifiy the directory entry contents
            pair<bool, SInt32> add_result = directory_entry->addSharer(requester);
            assert(add_result.first == true);
            directory_block_info->setDState(DirectoryState::SHARED);

            retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, requester, address, cached_data_buf);
         }
         break;
   
      default:
         LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
         break;
   }
}

void
DramDirectoryCntlr::retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type,
      core_id_t receiver, IntPtr address, Byte* cached_data_buf)
{
   if (cached_data_buf != NULL)
   {
      // I already have the data I need cached
      getMemoryManager()->sendMsg(reply_msg_type, 
            MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
            receiver /* requester */, 
            receiver /* receiver */, 
            address, 
            cached_data_buf, getCacheBlockSize());
      
      // Process Next Request
      processNextReqFromL2Cache(address);
   }
   else
   {
      // Get the data from DRAM
      // This could be directly forwarded to the cache or passed
      // through the Dram Directory Controller

      // I have to get the data from DRAM
      core_id_t dram_cntlr_core_id = getMemoryManager()->getCoreIdFromDramCntlrNum(m_dram_home_lookup->getHome(address));
      if (dram_cntlr_core_id == m_core_id)
      {
         Byte data_buf[getCacheBlockSize()];
         // Dram Cntlr is attached to the same core
         m_dram_cntlr->getDataFromDram(address, receiver, data_buf);
         getMemoryManager()->sendMsg(reply_msg_type, 
               MemComponent::DRAM_DIR, MemComponent::L2_CACHE, 
               receiver /* requester */,
               receiver /* receiver */,
               address, 
               data_buf, getCacheBlockSize());

         // Process Next Request
         processNextReqFromL2Cache(address);
      }
      else
      {
         // Dram Cntlr is on another core
         getMemoryManager()->sendMsg(ShmemMsg::GET_DATA_REQ, 
               MemComponent::DRAM_DIR, MemComponent::DRAM, 
               receiver /* requester */,
               dram_cntlr_core_id /* receiver */, 
               address);
         m_dram_request_pending[address] = true;
      }
   }
}

void
DramDirectoryCntlr::processInvRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::SHARED);

   directory_entry->removeSharer(sender);
   if (directory_entry->numSharers() == 0)
   {
      directory_block_info->setDState(DirectoryState::UNCACHED);
   }

   if (m_dram_directory_req_queue_list->size(address) > 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update Times in the Shmem Perf Model and the Shmem Req
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
      {
         // An ShmemMsg::EX_REQ caused the invalidation
         if (directory_block_info->getDState() == DirectoryState::UNCACHED)
         {
            processExReqFromL2Cache(shmem_req);
         }
      }
      else // (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
      {
         // A ShmemMsg::SH_REQ caused the invalidation
         processShReqFromL2Cache(shmem_req);
      }
   }
}

void
DramDirectoryCntlr::processFlushRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   assert(directory_block_info->getDState() == DirectoryState::EXCLUSIVE);

   directory_entry->removeSharer(sender);
   directory_entry->setOwner(INVALID_CORE_ID);
   directory_block_info->setDState(DirectoryState::UNCACHED);

   // Write Data to Dram
   sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());

   if (m_dram_directory_req_queue_list->size(address) != 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update times
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      // An involuntary/voluntary Flush
      if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
      {
         processExReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
      else // (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
      {
         processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
      }
   }
}

void
DramDirectoryCntlr::processWbRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();

   DirectoryEntry* directory_entry = m_dram_directory_cache->getDirectoryEntry(address);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();

   assert(directory_block_info->getDState() == DirectoryState::EXCLUSIVE);
   assert(directory_entry->hasSharer(sender));
   
   directory_entry->setOwner(INVALID_CORE_ID);
   directory_block_info->setDState(DirectoryState::SHARED);

   // Write Data to Dram
   sendDataToDram(address, shmem_msg->getRequester(), shmem_msg->getDataBuf());

   if (m_dram_directory_req_queue_list->size(address) != 0)
   {
      ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

      // Update Time
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      assert(shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ);
      processShReqFromL2Cache(shmem_req, shmem_msg->getDataBuf());
   }
}

void
DramDirectoryCntlr::sendDataToDram(IntPtr address, core_id_t requester, Byte* data_buf)
{
   // Write data to Dram
   SInt32 dram_cntlr_core_id = getMemoryManager()->getCoreIdFromDramCntlrNum(m_dram_home_lookup->getHome(address));
   if (dram_cntlr_core_id == m_core_id)
   {
      // Dram Cntlr is attached to the same core
      m_dram_cntlr->putDataToDram(address, requester, data_buf);
   }
   else
   {
      // Dram Cntlr is attached to a different core
      getMemoryManager()->sendMsg(ShmemMsg::PUT_DATA_REQ, 
            MemComponent::DRAM_DIR, MemComponent::DRAM, 
            requester /* requester */,
            dram_cntlr_core_id /* receiver */, 
            address, 
            data_buf, getCacheBlockSize());
   }
}

void
DramDirectoryCntlr::handleMsgFromDram(ShmemMsg* shmem_msg)
{
   assert(shmem_msg->getMsgType() == ShmemMsg::GET_DATA_REP);

   processGetDataRepFromDram(shmem_msg);
}

void
DramDirectoryCntlr::processGetDataRepFromDram(ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   assert(m_dram_directory_req_queue_list->size(address) > 0);
   assert(shmem_msg->getDataBuf() != NULL);

   ShmemReq* shmem_req = m_dram_directory_req_queue_list->front(address);

   // Update Time
   shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
   getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

   m_dram_request_pending[address] = false;

   if (shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::EX_REQ)
      retrieveDataAndSendToL2Cache(ShmemMsg::EX_REP, shmem_msg->getRequester(), address, shmem_msg->getDataBuf());
   else // shmem_req->getShmemMsg()->getMsgType() == ShmemMsg::SH_REQ)
      retrieveDataAndSendToL2Cache(ShmemMsg::SH_REP, shmem_msg->getRequester(), address, shmem_msg->getDataBuf());
}

}
