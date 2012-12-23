#pragma once

#include <iostream>
#include <fstream>
using std::ofstream;

#include "../memory_manager.h"
#include "cache.h"
#include "l1_cache_cntlr.h"
#include "l2_cache_cntlr.h"
#include "dram_directory_cntlr.h"
#include "dram_cntlr.h"
#include "address_home_lookup.h"
#include "shmem_msg.h"
#include "mem_component.h"
#include "lock.h"
#include "semaphore.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "network.h"

namespace PrL1PrL2DramDirectoryMOSI
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
      
      // Update internal variables when frequency is changed
      // Variables that need to be updated include all variables that are expressed in terms of cycles
      //  e.g., total memory access latency, packet arrival time, etc.
      void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);
      
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

      void outputSummary(std::ostream &os);
      
      // Cache line replication trace
      static void openCacheLineReplicationTraceFiles();
      static void closeCacheLineReplicationTraceFiles();
      static void outputCacheLineReplicationSummary();
      
      void incrCycleCount(MemComponent::Type mem_component, CachePerfModel::CacheAccess_t access_type);

   private:
      L1CacheCntlr* _L1_cache_cntlr;
      L2CacheCntlr* _L2_cache_cntlr;
      DramDirectoryCntlr* _dram_directory_cntlr;
      DramCntlr* _dram_cntlr;
      
      // Home Lookups
      AddressHomeLookup* _dram_directory_home_lookup;

      bool _dram_cntlr_present;

      UInt32 _cache_line_size;

      // Performance Models
      CachePerfModel* _L1_icache_perf_model;
      CachePerfModel* _L1_dcache_perf_model;
      CachePerfModel* _L2_cache_perf_model;

      // If TRUE, use two networks to communicate shared memory messages.
      // If FALSE, use just one network
      bool _switch_networks;

      // Cache Line Replication
      static ofstream _cache_line_replication_file;

      bool coreInitiateMemoryAccess(MemComponent::Type mem_component,
                                    Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type,
                                    IntPtr address, UInt32 offset, Byte* data_buf, UInt32 data_length,
                                    bool modeled);

      void handleMsgFromNetwork(NetPacket& packet);

      // Get Packet Type for a message
      PacketType getPacketType(MemComponent::Type sender_mem_component, MemComponent::Type receiver_mem_component);
   };
}
