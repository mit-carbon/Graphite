#pragma once

#include "../memory_manager.h"
#include "cache.h"
#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "dram_directory_cntlr.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"

namespace ShL1ShL2
{
   class MemoryManager : public ::MemoryManager
   {
   public:
      MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model);
      ~MemoryManager();

      UInt32 getCacheLineSize() { return _cache_line_size; }

      Cache* getL2Cache() { return _l2_cache_cntlr->getL2Cache(); }
      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cntlr->getDramDirectoryCache(); }
      DramCntlr* getDramCntlr() { return _dram_cntlr; }
      AddressHomeLookup* getAddressHomeLookup() { return _address_home_lookup; }

      bool coreInitiateMemoryAccess(MemComponent::component_t mem_component,
            Core::lock_signal_t lock_signal,
            Core::mem_op_t mem_op_type,
            IntPtr address, UInt32 offset,
            Byte* data_buf, UInt32 data_length,
            bool modeled);

      void handleMsgFromNetwork(NetPacket& packet);

      // Send/Broadcast msg
      void sendMsg(tile_id_t receiver, ShmemMsg& msg);
     
      void enableModels();
      void disableModels();

      tile_id_t getShmemRequester(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getRequester(); }
      UInt32 getModeledLength(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getModeledLength(); }
      bool isModeled(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->isModeled(); }

      void outputSummary(std::ostream &os);

      void incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type);
   
   private:
      L1CacheCntlr* _l1_cache_cntlr;
      L2CacheCntlr* _l2_cache_cntlr;
      DramDirectoryCntlr* _dram_directory_cntlr;
      DramCntlr* _dram_cntlr;
      AddressHomeLookup* _address_home_lookup;

      UInt32 _cache_line_size;
      bool _enabled;

      // Performance Models
      CachePerfModel* _l1_icache_perf_model;
      CachePerfModel* _l1_dcache_perf_model;
      CachePerfModel* _l2_cache_perf_model;
      CachePerfModel* _dram_directory_cache_perf_model;
   };
}
