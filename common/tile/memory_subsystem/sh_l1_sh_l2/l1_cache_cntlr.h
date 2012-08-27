#pragma once

#include <string>
using std::string;

// Forward declaration
namespace ShL1ShL2
{
   class MemoryManager;
}

#include "tile.h"
#include "cache.h"
#include "cache_line_info.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "fixed_types.h"
#include "address_home_lookup.h"
#include "shmem_perf_model.h"

namespace ShL1ShL2
{
   class L1CacheCntlr
   {
   public:
      L1CacheCntlr(MemoryManager* memory_manager,
                   AddressHomeLookup* address_home_lookup,
                   UInt32 cache_line_size,
                   float frequency);
      ~L1CacheCntlr();

      bool processMemOpFromTile(MemComponent::Type mem_component,
            Core::lock_signal_t lock_signal,
            Core::mem_op_t mem_op_type, 
            IntPtr ca_address, UInt32 offset,
            Byte* data_buf, UInt32 data_length,
            bool modeled);

      void handleMsgFromDramDirectory(tile_id_t sender, ShmemMsg* shmem_msg);

   private:
      MemoryManager* _memory_manager;
      AddressHomeLookup* _address_home_lookup;

      Byte* _outstanding_data_buf;
      Semaphore _app_thread_sem;
      
      tile_id_t getTileId();
      UInt32 getCacheLineSize();
      MemoryManager* getMemoryManager()   { return _memory_manager; }
      ShmemPerfModel* getShmemPerfModel();

      void waitForNetworkThread();
      void wakeUpAppThread();
   };
}
