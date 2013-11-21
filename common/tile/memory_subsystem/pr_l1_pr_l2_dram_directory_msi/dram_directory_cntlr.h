#pragma once

#include <string>
using std::string;

// Forward Decls
namespace PrL1PrL2DramDirectoryMSI
{
   class MemoryManager;
}

#include "directory_cache.h"
#include "hash_map_list.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class DramDirectoryCntlr
   {
   public:
      DramDirectoryCntlr(MemoryManager* memory_manager,
            DramCntlr* dram_cntlr,
            string dram_directory_total_entries_str,
            UInt32 dram_directory_associativity,
            UInt32 cache_block_size,
            UInt32 dram_directory_max_num_sharers,
            UInt32 dram_directory_max_hw_sharers,
            string dram_directory_type_str,
            string dram_directory_access_cycles_str,
            UInt32 num_dram_cntlrs);
      ~DramDirectoryCntlr();

      void handleMsgFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);

      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cache; }
   
   private:
      // Functional Models
      MemoryManager* _memory_manager;
      DirectoryCache* _dram_directory_cache;
      DramCntlr* _dram_cntlr;
      HashMapList<IntPtr,ShmemReq*> _dram_directory_req_queue;

      UInt32 getCacheLineSize();
      ShmemPerfModel* getShmemPerfModel();

      // Private Functions
      DirectoryEntry* processDirectoryEntryAllocationReq(ShmemReq* shmem_req);
      void processNullifyReq(ShmemReq* shmem_req);

      void processNextReqFromL2Cache(IntPtr address);
      void processExReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf = NULL);
      void processShReqFromL2Cache(ShmemReq* shmem_req, Byte* cached_data_buf = NULL);
      void retrieveDataAndSendToL2Cache(ShmemMsg::Type reply_msg_type, tile_id_t receiver, IntPtr address, Byte* cached_data_buf, bool msg_modeled);

      void processInvRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processFlushRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processWbRepFromL2Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void sendDataToDram(IntPtr address, Byte* data_buf, bool msg_modeled);
   };
}
