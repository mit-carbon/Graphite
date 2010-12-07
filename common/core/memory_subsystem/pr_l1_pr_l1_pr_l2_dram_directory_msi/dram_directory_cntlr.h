#pragma once

#include <string>

// Forward Decls
namespace PrL1PrL1PrL2DramDirectoryMSI
{
   class MemoryManager;
}

#include "dram_directory_cache.h"
#include "req_queue_list.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"

namespace PrL1PrL1PrL2DramDirectoryMSI
{
   class DramDirectoryCntlr
   {
      private:
         // Functional Models
         MemoryManager* m_memory_manager;
         DramDirectoryCache* m_dram_directory_cache;
         ReqQueueList* m_dram_directory_req_queue_list;
         DramCntlr* m_dram_cntlr;

         tile_id_t m_tile_id;
         UInt32 m_cache_block_size;

         ShmemPerfModel* m_shmem_perf_model;

         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         MemoryManager* getMemoryManager() { return m_memory_manager; }
         ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

         // Private Functions
         DirectoryEntry* processDirectoryEntryAllocationReq(ShmemReq* shmem_req);
         void processNullifyReq(ShmemReq* shmem_req);

         void processNextReqFromL2Cache(IntPtr address);
         void processExReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf = NULL);
         void processShReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf = NULL);
         void retrieveDataAndSendToL2Cache(ShmemMsg::msg_t reply_msg_type, tile_id_t receiver, IntPtr address, Byte* cached_data_buf);

         void processInvRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
         void processFlushRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
         void processWbRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
         void sendDataToDram(IntPtr address, tile_id_t requester, Byte* data_buf);
      
      public:
         DramDirectoryCntlr(tile_id_t tile_id,
               MemoryManager* memory_manager,
               DramCntlr* dram_cntlr,
               UInt32 dram_directory_total_entries,
               UInt32 dram_directory_associativity,
               UInt32 cache_block_size,
               UInt32 dram_directory_max_num_sharers,
               UInt32 dram_directory_max_hw_sharers,
               std::string dram_directory_type_str,
               UInt64 dram_directory_cache_access_delay_in_ns,
               UInt32 num_dram_cntlrs,
               ShmemPerfModel* shmem_perf_model);
         ~DramDirectoryCntlr();

         void handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
         
         DramDirectoryCache* getDramDirectoryCache() { return m_dram_directory_cache; }
   };

}
