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
#include "semaphore.h"
#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "network.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class MemoryManager : public ::MemoryManager
   {
   public:
      MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model);
      ~MemoryManager();

      UInt32 getCacheLineSize() { return _cache_line_size; }

      Cache* getL1ICache() { return _l1_cache_cntlr->getL1ICache(); }
      Cache* getL1DCache() { return _l1_cache_cntlr->getL1DCache(); }
      Cache* getL2Cache() { return _l2_cache_cntlr->getL2Cache(); }
      DirectoryCache* getDramDirectoryCache() { return _dram_directory_cntlr->getDramDirectoryCache(); }
      DramCntlr* getDramCntlr() { return _dram_cntlr; }
      bool isDramCntlrPresent() { return _dram_cntlr_present; }
      AddressHomeLookup* getDramDirectoryHomeLookup() { return _dram_directory_home_lookup; }
      
      bool coreInitiateMemoryAccess(
            MemComponent::component_t mem_component,
            Core::lock_signal_t lock_signal,
            Core::mem_op_t mem_op_type,
            IntPtr address, UInt32 offset,
            Byte* data_buf, UInt32 data_length,
            bool modeled);

      void handleMsgFromNetwork(NetPacket& packet);

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
      
      void incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type);

   private:
      L1CacheCntlr* _l1_cache_cntlr;
      L2CacheCntlr* _l2_cache_cntlr;
      DramDirectoryCntlr* _dram_directory_cntlr;
      DramCntlr* _dram_cntlr;
      AddressHomeLookup* _dram_directory_home_lookup;

      bool _dram_cntlr_present;

      Semaphore* _user_thread_sem;
      Semaphore* _network_thread_sem;

      UInt32 _cache_line_size;
      bool _enabled;

      // Packet Types for different kinds of traffic - See comment in carbon_sim.cfg
      SInt32 _unicast_threshold;
      PacketType _unicast_packet_type_lt_threshold;
      PacketType _unicast_packet_type_ge_threshold;
      PacketType _broadcast_packet_type;

      // Performance Models
      CachePerfModel* _l1_icache_perf_model;
      CachePerfModel* _l1_dcache_perf_model;
      CachePerfModel* _l2_cache_perf_model;

      // Cache Line Replication
      static ofstream _cache_line_replication_file;

      // Get Packet Type for a message
      PacketType getPacketType(tile_id_t sender, tile_id_t receiver);
      // Parse Network Type
      PacketType parseNetworkType(std::string& network_type);
   };
}
