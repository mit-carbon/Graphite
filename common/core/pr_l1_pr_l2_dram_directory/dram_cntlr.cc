#include <string.h>
using namespace std;

#include "dram_cntlr.h"
#include "memory_manager.h"
#include "core.h"
#include "log.h"

namespace PrL1PrL2DramDirectory
{

DramCntlr::DramCntlr(MemoryManager* memory_manager,
      float dram_access_cost,
      float dram_bandwidth,
      float core_frequency,
      bool dram_queue_model_enabled,
      bool dram_queue_model_moving_avg_enabled,
      UInt32 dram_queue_model_moving_avg_window_size,
      string dram_queue_model_moving_avg_type, 
      UInt32 cache_block_size,
      ShmemPerfModel* shmem_perf_model):
   m_memory_manager(memory_manager),
   m_cache_block_size(cache_block_size),
   m_shmem_perf_model(shmem_perf_model)
{
   m_dram_perf_model = new DramPerfModel(dram_access_cost, 
         dram_bandwidth,
         core_frequency, 
         dram_queue_model_enabled, 
         dram_queue_model_moving_avg_enabled,
         dram_queue_model_moving_avg_window_size, 
         dram_queue_model_moving_avg_type);
}

DramCntlr::~DramCntlr()
{
   delete m_dram_perf_model;
}

void
DramCntlr::handleMsgFromDramDir(core_id_t sender, ShmemMsg* shmem_msg)
{
   IntPtr address = shmem_msg->getAddress();
   ShmemMsg::msg_t msg_type = shmem_msg->getMsgType();
   core_id_t requester = shmem_msg->getRequester();

   switch (msg_type)
   {
      case ShmemMsg::GET_DATA_REQ:
         {
            Byte data_buf[getCacheBlockSize()];
            getDataFromDram(address, requester, data_buf);
            getMemoryManager()->sendMsg(ShmemMsg::GET_DATA_REP, 
                  MemComponent::DRAM, MemComponent::DRAM_DIR, 
                  requester /* requester */, 
                  sender /* receiver */, 
                  address,
                  data_buf, getCacheBlockSize());
         }
         break;

      case ShmemMsg::PUT_DATA_REQ:
         putDataToDram(address, requester, shmem_msg->getDataBuf());
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized Shmem Msg Type(%u)", msg_type);
         break;
   }
}

void
DramCntlr::getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf)
{
   if (m_data_map[address] == NULL)
   {
      m_data_map[address] = new Byte[getCacheBlockSize()];
      memset((void*) m_data_map[address], 0x00, getCacheBlockSize());
   }
   memcpy((void*) data_buf, (void*) m_data_map[address], getCacheBlockSize());

   UInt64 dram_access_latency = runDramPerfModel(requester);
   getShmemPerfModel()->incrCycleCount(dram_access_latency);
}

void
DramCntlr::putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf)
{
   if (m_data_map[address] == NULL)
   {
      LOG_PRINT_ERROR("Data Buffer does not exist");
   }
   memcpy((void*) m_data_map[address], (void*) data_buf, getCacheBlockSize());

   runDramPerfModel(requester);
}

UInt64
DramCntlr::runDramPerfModel(core_id_t requester)
{
   UInt64 pkt_time = getShmemPerfModel()->getCycleCount();
   UInt64 pkt_size = (UInt64) getCacheBlockSize();
   UInt64 dram_access_latency = m_dram_perf_model->getAccessLatency(pkt_time, pkt_size, requester);
   return dram_access_latency;
}

}
