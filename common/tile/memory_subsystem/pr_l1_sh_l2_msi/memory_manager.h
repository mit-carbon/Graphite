#pragma once

#include <iostream>
#include <fstream>
using std::ofstream;

#include "../memory_manager.h"
#include "cache.h"
#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "semaphore.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "network.h"
#include "dvfs.h"

namespace PrL1ShL2MSI
{
   class MemoryManager : public ::MemoryManager
   {
   public:
      MemoryManager(Tile* tile);
      ~MemoryManager();

      UInt32 getCacheLineSize() { return _cache_line_size; }

      Cache* getL1ICache() { return _L1_cache_cntlr->getL1ICache(); }
      Cache* getL1DCache() { return _L1_cache_cntlr->getL1DCache(); }
      Cache* getL2Cache() { return _L2_cache_cntlr->getL2Cache(); }
      DramCntlr* getDramCntlr() { return _dram_cntlr; }
      bool isDramCntlrPresent() { return _dram_cntlr_present; }
      
      void sendMsg(tile_id_t receiver, ShmemMsg& shmem_msg);
      void broadcastMsg(ShmemMsg& shmem_msg);
    
      void enableModels();
      void disableModels();

      UInt32 getModeledLength(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getModeledLength(); }
      bool isModeled(const void* pkt_data)
      { return  ((ShmemMsg*) pkt_data)->isModeled(); }
      tile_id_t getShmemRequester(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getRequester(); }

      void outputSummary(std::ostream &os, const Time& target_completion_time);

      // Energy monitoring
      void computeEnergy(const Time& curr_time);

      double getDynamicEnergy();
      double getLeakageEnergy();

      void incrCurrTime(MemComponent::Type mem_component, CachePerfModel::AccessType access_type);

      int getDVFS(module_t module, double &frequency, double &voltage);
      int setDVFS(module_t module, double frequency, voltage_option_t voltage_flag, const Time& curr_time);

   private:
      // L1/L2 cache cntlrs and DRAM_CNTLR cntlr
      L1CacheCntlr* _L1_cache_cntlr;
      L2CacheCntlr* _L2_cache_cntlr;
      DramCntlr* _dram_cntlr;
      bool _dram_cntlr_present;

      // Home Lookups
      AddressHomeLookup* _L2_cache_home_lookup;
      AddressHomeLookup* _dram_home_lookup;

      UInt32 _cache_line_size;

      bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                    Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                    IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length,
                                    bool modeled);

      void handleMsgFromNetwork(NetPacket& packet);
   };
}
