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
#include "dvfs.h"


namespace PrL1PrL2DramDirectoryMSI
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
      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cntlr->getDramDirectoryCache(); }
      DramCntlr* getDramCntlr() { return _dram_cntlr; }
      bool isDramCntlrPresent() { return _dram_cntlr_present; }
      AddressHomeLookup* getDramDirectoryHomeLookup() { return _dram_directory_home_lookup; }

      // Send/Broadcast msg
      void sendMsg(tile_id_t receiver, ShmemMsg& msg);
      void broadcastMsg(ShmemMsg& msg);
     
      void enableModels();
      void disableModels();

      tile_id_t getShmemRequester(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getRequester(); }
      UInt32 getModeledLength(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->getModeledLength(); }
      bool isModeled(const void* pkt_data)
      { return ((ShmemMsg*) pkt_data)->isModeled(); }

      void outputSummary(std::ostream &os, const Time& target_completion_time);

      // Energy monitoring
      void computeEnergy(const Time& curr_time);

      double getDynamicEnergy();
      double getLeakageEnergy();

      void incrCurrTime(MemComponent::Type mem_component, CachePerfModel::AccessType access_type);
      int getDVFS(module_t module, double &frequency, double &voltage);
      int setDVFS(module_t module, double frequency, voltage_option_t voltage_flag, const Time& curr_time);
   
   private:
      L1CacheCntlr* _L1_cache_cntlr;
      L2CacheCntlr* _L2_cache_cntlr;
      DramDirectoryCntlr* _dram_directory_cntlr;
      DramCntlr* _dram_cntlr;

      // Home lookups
      AddressHomeLookup* _dram_directory_home_lookup;

      bool _dram_cntlr_present;

      UInt32 _cache_line_size;

      bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                    Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                    IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length,
                                    bool modeled);

      void handleMsgFromNetwork(NetPacket& packet);
   };
}
