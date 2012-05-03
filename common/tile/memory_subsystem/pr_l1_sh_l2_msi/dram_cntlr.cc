#include "dram_cntlr.h"
#include "memory_manager.h"
#include "log.h"

namespace PrL1ShL2MSI
{

DramCntlr::DramCntlr(Tile* tile,
                     float dram_access_cost, float dram_bandwidth,
                     bool dram_queue_model_enabled, string dram_queue_model_type,
                     UInt32 cache_line_size)
   : ::DramCntlr(tile, dram_access_cost, dram_bandwidth, dram_queue_model_enabled, dram_queue_model_type, cache_line_size)
   , _memory_manager((MemoryManager*) tile->getMemoryManager())
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

   switch (shmem_msg_type)
   {
   case ShmemMsg::GET_DATA_REQ:
      {
         Byte data_buf[_cache_line_size];
         getDataFromDram(address, data_buf, msg_modeled);
         ShmemMsg dram_reply(ShmemMsg::GET_DATA_REP, MemComponent::DRAM, MemComponent::L2_CACHE,
                             requester, false, address,
                             data_buf, _cache_line_size,
                             msg_modeled);
         _memory_manager->sendMsg(sender, dram_reply);
      }
      break;

   case ShmemMsg::PUT_DATA_REQ:
      putDataToDram(address, shmem_msg->getDataBuf(), msg_modeled);
      break;
   
   default:
      LOG_PRINT_ERROR("Unrecgonized Msg From L2 Cache(%u)", shmem_msg_type);
      break;
   }
}

}
