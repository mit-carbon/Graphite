#ifndef __DRAM_DIRECTORY_CNTLR_H__
#define __DRAM_DIRECTORY_CNTLR_H__

#include <string>

// Forward Decls
namespace PrL1PrL2DramDirectory
{
   class MemoryManager;
}

#include "dram_directory_cache.h"
#include "req_queue_list.h"
#include "dram_cntlr.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"

namespace PrL1PrL2DramDirectory
{
   class DramDirectoryCntlr
   {
      private:
         // Functional Models
         MemoryManager* m_memory_manager;
         DramDirectoryCache* m_dram_directory_cache;
         ReqQueueList* m_dram_directory_req_queue_list;
         DramCntlr* m_dram_cntlr;
         UInt32 m_cache_block_size;

         UInt32 getCacheBlockSize() { return m_cache_block_size; }
         MemoryManager* getMemoryManager() { return m_memory_manager; }

         // Private Functions
         void processNextReqFromL2Cache(IntPtr address);
         void processExReqFromL2Cache(ShmemReq* shmem_req);
         void processShReqFromL2Cache(ShmemReq* shmem_req);
         void processInvRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
         void processFlushRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
         void processWbRepFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg);
      
      public:
         DramDirectoryCntlr(MemoryManager* memory_manager,
               UInt32 dram_directory_total_entries,
               UInt32 dram_directory_associativity,
               UInt32 cache_block_size,
               UInt32 dram_directory_max_num_sharers,
               UInt32 dram_directory_max_hw_sharers,
               std::string dram_directory_type_str,
               DramCntlr* dram_cntlr);
         ~DramDirectoryCntlr();

         void handleMsgFromL2Cache(core_id_t sender, ShmemMsg* shmem_msg, UInt64 msg_time);

   };

}

#endif /* __DRAM_DIRECTORY_CNTLR_H__ */
