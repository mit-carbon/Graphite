#include "dram_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace PrL1ShL2MSI
{

DramCntlr::DramCntlr(MemoryManager* memory_manager,
                     float dram_access_cost, float dram_bandwidth,
                     bool dram_queue_model_enabled, string dram_queue_model_type,
                     UInt32 cache_line_size)
   : ::DramCntlr(memory_manager->getTile(), dram_access_cost, dram_bandwidth, dram_queue_model_enabled, dram_queue_model_type, cache_line_size)
   , _memory_manager(memory_manager)
{}

DramCntlr::~DramCntlr()
{}

void
DramCntlr::handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg)
{
   ShmemMsg::Type shmem_msg_type = shmem_msg->getType();
   IntPtr address = shmem_msg->getAddress();
   tile_id_t requester = shmem_msg->getRequester();
   bool msg_modeled = shmem_msg->isModeled();
   LOG_PRINT("handleMsgFromL2Cache(%i, %#lx, %s)", sender, address, msg_modeled ? "TRUE" : "FALSE");

   switch (shmem_msg_type)
   {
   case ShmemMsg::DRAM_FETCH_REQ:
      {
         Byte data_buf[_cache_line_size];
         getDataFromDram(address, data_buf, msg_modeled);
         LOG_PRINT("Finished fetching data from DRAM_CNTLR, sending reply");
         ShmemMsg dram_reply(ShmemMsg::DRAM_FETCH_REP, MemComponent::DRAM_CNTLR, MemComponent::L2_CACHE,
                             requester, false, address,
                             data_buf, _cache_line_size,
                             msg_modeled);
         _memory_manager->sendMsg(sender, dram_reply);
         LOG_PRINT("Sent reply");
      }
      break;

   case ShmemMsg::DRAM_STORE_REQ:
      putDataToDram(address, shmem_msg->getDataBuf(), msg_modeled);
      break;
   
   default:
      LOG_PRINT_ERROR("Unrecgonized Msg From L2 Cache(%u)", shmem_msg_type);
      break;
   }
}

}
