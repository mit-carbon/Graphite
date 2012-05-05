using namespace std;

#include "memory_manager.h"
#include "l2_cache_cntlr.h"
#include "dram_directory_cntlr.h"
#include "log.h"

namespace ShL1ShL2
{

DramDirectoryCntlr::DramDirectoryCntlr(MemoryManager* memory_manager,
                                       DramCntlr* dram_cntlr,
                                       UInt32 dram_directory_total_entries,
                                       UInt32 dram_directory_associativity,
                                       UInt32 cache_line_size,
                                       UInt32 dram_directory_max_num_sharers,
                                       UInt32 dram_directory_max_hw_sharers,
                                       string dram_directory_type_str,
                                       UInt64 dram_directory_cache_access_delay_in_ns,
                                       UInt32 num_dram_cntlrs)
   : _memory_manager(memory_manager)
   , _dram_cntlr(dram_cntlr)
   , _locked(false)
   , _lock_owner(INVALID_TILE_ID)
{
   _dram_directory_cache = new DirectoryCache(_memory_manager->getTile(),
                                              dram_directory_type_str,
                                              dram_directory_total_entries,
                                              dram_directory_associativity,
                                              cache_line_size,
                                              dram_directory_max_hw_sharers,
                                              dram_directory_max_num_sharers,
                                              num_dram_cntlrs,
                                              dram_directory_cache_access_delay_in_ns);
}

DramDirectoryCntlr::~DramDirectoryCntlr()
{
   delete _dram_directory_cache;
}

void
DramDirectoryCntlr::handleMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   if (!_locked)
   {
      assert(_request_queue.empty());
      processMsgFromL1Cache(sender, shmem_msg);
   }
   else if (sender == _lock_owner)
   {
      processMsgFromL1Cache(sender, shmem_msg);
      assert(!_locked);
      processPendingRequests();
   }
   else // ((_locked) && (sender != _lock_owner))
   {
      _request_queue.push(new ShmemReq(sender, shmem_msg, getShmemPerfModel()->getCycleCount()));
   }
}

void
DramDirectoryCntlr::processPendingRequests()
{
   while ( (!_request_queue.empty()) && (!_locked) )
   {
      ShmemReq* shmem_req = _request_queue.front();
      _request_queue.pop();

      // Update Times in the Shmem Perf Model and the Shmem Req
      shmem_req->updateTime(getShmemPerfModel()->getCycleCount());
      getShmemPerfModel()->updateCycleCount(shmem_req->getTime());

      processMsgFromL1Cache(shmem_req->getSender(), shmem_req->getShmemMsg());
      delete shmem_req;
   }
}

void
DramDirectoryCntlr::processMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();

   switch (shmem_msg_type)
   {
      case ShmemMsg::READ_REQ:
         processReadReqFromL1Cache(sender, shmem_msg);
         break;

      case ShmemMsg::READ_EX_REQ:
         processReadReqFromL1Cache(sender, shmem_msg);
         _locked = true;
         _lock_owner = sender;
         break;

      case ShmemMsg::WRITE_REQ:
         processWriteReqFromL1Cache(sender, shmem_msg);
         _locked = false;
         _lock_owner = INVALID_TILE_ID;
         break;
      
      default:
         LOG_PRINT_ERROR("Unrecognized Shmem Msg Type: %u", shmem_msg_type);
         break;
   }
   
   // Do Performance Modeling
   _memory_manager->incrCycleCount(MemComponent::L2_CACHE, CachePerfModel::ACCESS_CACHE_DATA_AND_TAGS);
   _memory_manager->incrCycleCount(MemComponent::DRAM_DIRECTORY, CachePerfModel::ACCESS_CACHE_DATA);
}

void
DramDirectoryCntlr::processReadReqFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   UInt32 offset = shmem_msg->getOffset();
   UInt32 size = shmem_msg->getSize();
   tile_id_t requester = shmem_msg->getRequester();

   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   LOG_ASSERT_ERROR(directory_entry, "Does not handle directory evictions for now");
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::UNCACHED:
      {
         Byte data_buf[getCacheLineSize()];
         _dram_cntlr->getDataFromDram(address, data_buf, true);
         _l2_cache_cntlr->insertCacheLine(address, CacheState::SHARED, data_buf);
         
         directory_entry->addSharer(getTileId());
         directory_block_info->setDState(DirectoryState::SHARED);
         break;
      }

   case DirectoryState::SHARED:
   case DirectoryState::MODIFIED:
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }
         
   Byte read_data[size];
   _l2_cache_cntlr->readData(address, offset, size, read_data);

   ShmemMsg msg(ShmemMsg::READ_REP, MemComponent::DRAM_DIRECTORY, MemComponent::L1_DCACHE, requester, address, offset, size,
                read_data, size);
   getMemoryManager()->sendMsg(sender, msg);
}

void
DramDirectoryCntlr::processWriteReqFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   UInt32 offset = shmem_msg->getOffset();
   UInt32 size = shmem_msg->getSize();
   tile_id_t requester = shmem_msg->getRequester();
   
   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   LOG_ASSERT_ERROR(directory_entry, "Does not handle directory evictions for now");
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::UNCACHED:
      {
         Byte data_buf[getCacheLineSize()];
         _dram_cntlr->getDataFromDram(address, data_buf, true);
         _l2_cache_cntlr->insertCacheLine(address, CacheState::MODIFIED, data_buf);
         
         directory_entry->addSharer(getTileId());
         directory_block_info->setDState(DirectoryState::MODIFIED);
         break;
      }

   case DirectoryState::SHARED:
      directory_block_info->setDState(DirectoryState::MODIFIED);
      _l2_cache_cntlr->setCacheLineState(address, CacheState::MODIFIED);
      break;

   case DirectoryState::MODIFIED:
      break;

   default:
      LOG_PRINT_ERROR("Unsupported Directory State: %u", curr_dstate);
      break;
   }
   
   _l2_cache_cntlr->writeData(address, offset, size, shmem_msg->getDataBuf());

   ShmemMsg msg(ShmemMsg::WRITE_REP, MemComponent::DRAM_DIRECTORY, MemComponent::L1_DCACHE, requester, address, offset, size);
   getMemoryManager()->sendMsg(sender, msg);
}

void
DramDirectoryCntlr::handleEvictedCacheLineFromL2Cache(IntPtr address, Byte* data_buf)
{
   DirectoryEntry* directory_entry = _dram_directory_cache->getDirectoryEntry(address);
   assert(directory_entry);
   DirectoryBlockInfo* directory_block_info = directory_entry->getDirectoryBlockInfo();
   DirectoryState::Type curr_dstate = directory_block_info->getDState();

   switch (curr_dstate)
   {
   case DirectoryState::MODIFIED:
      assert(data_buf);
      _dram_cntlr->putDataToDram(address, data_buf, true);
      break;

   case DirectoryState::SHARED:
      assert(!data_buf);
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized DirectoryState (%u)", curr_dstate);
      break;
   }
   
   directory_entry->removeSharer(getTileId());
   assert(directory_entry->getNumSharers() == 0);
   directory_block_info->setDState(DirectoryState::UNCACHED);
}

tile_id_t
DramDirectoryCntlr::getTileId()
{
   return _memory_manager->getTile()->getId();
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
