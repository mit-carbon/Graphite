#pragma once

// Forward declaration
namespace HybridProtocol_PPMOSI_SS
{
   class MemoryManager;
}

#include "../dram_cntlr.h"
#include "shmem_msg.h"
#include "tile.h"
#include "fixed_types.h"

namespace HybridProtocol_PPMOSI_SS
{

class DramCntlr : public ::DramCntlr
{
public:
   DramCntlr(MemoryManager* memory_manager,
             float dram_access_cost, float dram_bandwidth,
             bool dram_queue_model_enabled, string dram_queue_model_type,
             UInt32 cache_line_size);
   ~DramCntlr();

   void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);

private:
   MemoryManager* _memory_manager; 
};

}
