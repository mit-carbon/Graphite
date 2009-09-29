#ifndef __DRAM_CNTLR_H__
#define __DRAM_CNTLR_H__

#include <map>

// Forward Decls
namespace PrL1PrL2DramDirectory
{
   class MemoryManager;
}

#include "dram_perf_model.h"
#include "shmem_perf_model.h"
#include "shmem_msg.h"
#include "fixed_types.h"

namespace PrL1PrL2DramDirectory
{
   class DramCntlr
   {
      private:
         MemoryManager* m_memory_manager;
         std::map<IntPtr, Byte*> m_data_map;
         DramPerfModel* m_dram_perf_model;
         UInt32 m_cache_block_size;
         ShmemPerfModel* m_shmem_perf_model;

         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         MemoryManager* getMemoryManager() { return m_memory_manager; }
         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }
         UInt64 runDramPerfModel(core_id_t requester);

      public:
         DramCntlr(MemoryManager* memory_manager,
               float dram_access_cost,
               float dram_bandwidth,
               float core_frequency,
               bool dram_queue_model_enabled,
               UInt32 dram_queue_model_moving_avg_window_size,
               string dram_queue_model_moving_avg_type, 
               UInt32 cache_block_size,
               ShmemPerfModel* shmem_perf_model);

         ~DramCntlr();

         DramPerfModel* getDramPerfModel() { return m_dram_perf_model; }

         void handleMsgFromDramDir(core_id_t sender, ShmemMsg* shmem_msg);
         void getDataFromDram(IntPtr address, core_id_t requester, Byte* data_buf);
         void putDataToDram(IntPtr address, core_id_t requester, Byte* data_buf);
   };
}

#endif /* __DRAM_CNTLR_H__ */
