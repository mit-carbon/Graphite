#pragma once

#include <string>
#include <queue>
using std::string;
using std::queue;

// Forward Decls
namespace ShL1ShL2
{
   class MemoryManager;
   class L2CacheCntlr;
}

#include "directory_cache.h"
#include "dram_cntlr.h"
#include "shmem_req.h"
#include "shmem_msg.h"
#include "mem_component.h"

namespace ShL1ShL2
{
   class DramDirectoryCntlr
   {
   public:
      DramDirectoryCntlr(MemoryManager* memory_manager,
                         DramCntlr* dram_cntlr,
                         UInt32 dram_directory_total_entries,
                         UInt32 dram_directory_associativity,
                         UInt32 cache_block_size,
                         UInt32 dram_directory_max_num_sharers,
                         UInt32 dram_directory_max_hw_sharers,
                         string dram_directory_type_str,
                         UInt64 dram_directory_cache_access_delay_in_ns,
                         UInt32 num_dram_cntlrs);
      ~DramDirectoryCntlr();

      void setL2CacheCntlr(L2CacheCntlr* l2_cache_cntlr)
      { _l2_cache_cntlr = l2_cache_cntlr; }

      void handleMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void handleEvictedCacheLineFromL2Cache(IntPtr address, Byte* data_buf);
      
      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cache; }
   
   private:
      // Functional Models
      MemoryManager* _memory_manager;
      DirectoryCache* _dram_directory_cache;
      L2CacheCntlr* _l2_cache_cntlr;
      DramCntlr* _dram_cntlr;
      queue<ShmemReq*> _request_queue;
      bool _locked;
      tile_id_t _lock_owner;
      
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager() { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();

      // Private Functions
      void processPendingRequests();
      void processMsgFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processReadReqFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg);
      void processWriteReqFromL1Cache(tile_id_t sender, ShmemMsg* shmem_msg);
   };
}
