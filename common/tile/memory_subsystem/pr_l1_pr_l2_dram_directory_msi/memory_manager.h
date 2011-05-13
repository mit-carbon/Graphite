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

namespace PrL1PrL2DramDirectoryMSI
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

         // Performance Models
         CachePerfModel* m_l1_icache_perf_model;
         CachePerfModel* m_l1_dcache_perf_model;
         CachePerfModel* m_l2_cache_perf_model;

      public:
         MemoryManager(Tile* tile, Network* network, ShmemPerfModel* shmem_perf_model);
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

         void sendMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, tile_id_t requester, tile_id_t receiver, IntPtr address, Byte* data_buf = NULL, UInt32 data_length = 0);

         void broadcastMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, tile_id_t requester, IntPtr address, Byte* data_buf = NULL, UInt32 data_length = 0);
        
         void enableModels();
         void disableModels();
         void resetModels();

         tile_id_t getShmemRequester(const void* pkt_data)
         { return ((ShmemMsg*) pkt_data)->getRequester(); }

         UInt32 getModeledLength(const void* pkt_data)
         { return ((ShmemMsg*) pkt_data)->getModeledLength(); }
         bool isModeled(const void* pkt_data)
         { return true; }

         void outputSummary(std::ostream &os);

         void incrCycleCount(MemComponent::component_t mem_component, CachePerfModel::CacheAccess_t access_type);
   };
}
