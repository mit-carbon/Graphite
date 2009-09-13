#ifndef __MEMORY_MANAGER_H__
#define __MEMORY_MANAGER_H__

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

namespace PrL1PrL2DramDirectory
{
   class MemoryManager : public MemoryManagerBase
   {
      private:
         L1CacheCntlr* m_l1_cache_cntlr;
         L2CacheCntlr* m_l2_cache_cntlr;
         DramDirectoryCntlr* m_dram_directory_cntlr;
         DramCntlr* m_dram_cntlr;
         AddressHomeLookup* m_dram_directory_home_lookup;

         Semaphore* m_mmu_sem;
         UInt32 m_cache_block_size;

      public:
         MemoryManager(Core* core, Network* network);
         ~MemoryManager();

         UInt32 getCacheBlockSize() { return m_cache_block_size; }

         bool coreInitiateMemoryAccess(
               MemComponent::component_t mem_component,
               Core::lock_signal_t lock_signal,
               Core::mem_op_t mem_op_type,
               IntPtr address, UInt32 offset,
               Byte* data_buf, UInt32 data_length);

         void handleMsgFromNetwork(NetPacket& packet);

         void sendMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, core_id_t receiver, IntPtr address, Byte* data_buf = NULL, UInt32 data_length = 0);

         void broadcastMsg(ShmemMsg::msg_t msg_type, MemComponent::component_t sender_mem_component, MemComponent::component_t receiver_mem_component, IntPtr address, Byte* data_buf = NULL, UInt32 data_length = 0);

   };
}

#endif /* __MEMORY_MANAGER_H__ */
