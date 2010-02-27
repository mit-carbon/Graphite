#ifndef __MEMORY_MANAGER_BASE_H__
#define __MEMORY_MANAGER_BASE_H__

#include "core.h"
#include "network.h"
#include "mem_component.h"
#include "shmem_perf_model.h"

void MemoryManagerNetworkCallback(void* obj, NetPacket packet);

class MemoryManagerBase
{
   public:
      enum CachingProtocol_t
      {
         PR_L1_PR_L2_DRAM_DIRECTORY_MSI = 0,
         PR_L1_PR_L2_DRAM_DIRECTORY_MOSI,
         NUM_CACHING_PROTOCOL_TYPES
      };

   protected:
      Core* m_core;
      Network* m_network;
      ShmemPerfModel* m_shmem_perf_model;

   public:
      MemoryManagerBase(Core* core, Network* network, ShmemPerfModel* shmem_perf_model):
         m_core(core), 
         m_network(network), 
         m_shmem_perf_model(shmem_perf_model)
      {}
      virtual ~MemoryManagerBase() {}

      virtual bool coreInitiateMemoryAccess(
            MemComponent::component_t mem_component,
            Core::lock_signal_t lock_signal,
            Core::mem_op_t mem_op_type,
            IntPtr address, UInt32 offset,
            Byte* data_buf, UInt32 data_length,
            bool modeled) = 0;

      virtual void handleMsgFromNetwork(NetPacket& packet) = 0;

      // FIXME: Take this out of here
      virtual UInt32 getCacheBlockSize() = 0;

      virtual void outputSummary(std::ostream& os) = 0;

      virtual void enableModels() = 0;
      virtual void disableModels() = 0;

      Core* getCore() { return m_core; }
      Network* getNetwork() { return m_network; }
      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

      static CachingProtocol_t parseProtocolType(std::string& protocol_type);
      static MemoryManagerBase* createMMU(std::string protocol_type,
            Core* core,
            Network* network, 
            ShmemPerfModel* shmem_perf_model);
};

#endif /* __MEMORY_MANAGER_BASE_H__ */
