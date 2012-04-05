#pragma once

#include <string>
using std::string;

// Forward declarations
namespace ShL1ShL2
{
   class DramDirectoryCntlr;
   class MemoryManager;
}

#include "cache.h"
#include "pr_l2_cache_line_info.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"

namespace ShL1ShL2
{
   class L2CacheCntlr
   {
   public:
      L2CacheCntlr(MemoryManager* memory_manager,
                   UInt32 cache_line_size,
                   UInt32 l2_cache_size,
                   UInt32 l2_cache_associativity,
                   string l2_cache_replacement_policy,
                   UInt32 l2_cache_access_delay,
                   bool l2_cache_track_miss_types,
                   volatile float frequency);
      ~L2CacheCntlr();

      void setDramDirectoryCntlr(DramDirectoryCntlr* dram_directory_cntlr)
      { _dram_directory_cntlr = dram_directory_cntlr; }

      Cache* getL2Cache() { return _l2_cache; }

      void setCacheLineState(IntPtr address, CacheState::Type cstate);
      void readData(IntPtr address, UInt32 offset, UInt32 size, Byte* data_buf);
      void writeData(IntPtr address, UInt32 offset, UInt32 size, Byte* data_buf);
      void insertCacheLine(IntPtr address, CacheState::Type cstate, Byte* fill_buf);

   private:
      // Data Members
      MemoryManager* _memory_manager;
      DramDirectoryCntlr* _dram_directory_cntlr;
      Cache* _l2_cache;
      
      // Utilities
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager()   { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();
   };

}
