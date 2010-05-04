#pragma once

#include "memory_manager_base.h"
#include "cache_base.h"
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
   class MemoryManager : public MemoryManagerBase
   {
      private:
         L1CacheCntlr* m_l1_cache_cntlr;
         L2CacheCntlr* m_l2_cache_cntlr;
         DramDirectoryCntlr* m_dram_directory_cntlr;
         DramCntlr* m_dram_cntlr;
         AddressHomeLookup* m_dram_directory_home_lookup;

         bool m_dram_cntlr_present;

         Semaphore* m_user_thread_sem;
         Semaphore* m_network_thread_sem;

         UInt32 m_cache_block_size;
         bool m_enabled;

         // Packet Types for different kinds of traffic - See comment in carbon_sim.cfg
         SInt32 m_unicast_threshold;
         PacketType m_unicast_packet_type_lt_threshold;
         PacketType m_unicast_packet_type_ge_threshold;
         PacketType m_broadcast_packet_type;

         // Performance Models
         CachePerfModel* m_l1_icache_perf_model;
         CachePerfModel* m_l1_dcache_perf_model;
         CachePerfModel* m_l2_cache_perf_model;

         // Get Packet Type for a message
         PacketType getPacketType(core_id_t sender, core_id_t receiver);
         // Parse Network Type
         PacketType parseNetworkType(std::string& network_type);

      public:
         MemoryManager(Core* core, Network* network, ShmemPerfModel* shmem_perf_model);
         ~MemoryManager();

         UInt32 getCacheBlockSize() { return m_cache_block_size; }

         Cache* getL1ICache() { return m_l1_cache_cntlr->getL1ICache(); }
         Cache* getL1DCache() { return m_l1_cache_cntlr->getL1DCache(); }
         Cache* getL2Cache() { return m_l2_cache_cntlr->getL2Cache(); }
         DramDirectoryCache* getDramDirectoryCache() { return m_dram_directory_cntlr->getDramDirectoryCache(); }
         DramCntlr* getDramCntlr() { return m_dram_cntlr; }
         AddressHomeLookup* getDramDirectoryHomeLookup() { return m_dram_directory_home_lookup; }

         bool coreInitiateMemoryAccess(
               MemComponent::component_t mem_component,
               Core::lock_signal_t lock_signal,
               Core::mem_op_t mem_op_type,
               IntPtr address, UInt32 offset,
               Byte* data_buf, UInt32 data_length,
               bool modeled);

         void handleMsgFromNetwork(NetPacket& packet);

         void sendMsg(core_id_t receiver, ShmemMsg& shmem_msg);
         void broadcastMsg(ShmemMsg& shmem_msg);
         
         void enableModels();
         void disableModels();

         UInt32 getModeledLength(const void* pkt_data)
         { return ((ShmemMsg*) pkt_data)->getModeledLength(); }

         core_id_t getShmemRequester(const void* pkt_data)
         { return ((ShmemMsg*) pkt_data)->getRequester(); }

         void outputSummary(std::ostream &os);

         void incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type);
   };
}
