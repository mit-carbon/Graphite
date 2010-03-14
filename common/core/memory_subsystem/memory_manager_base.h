#ifndef __MEMORY_MANAGER_BASE_H__
#define __MEMORY_MANAGER_BASE_H__

using namespace std;

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

   private:
      Core* m_core;
      Network* m_network;
      ShmemPerfModel* m_shmem_perf_model;
      
      void parseMemoryControllerList(string& memory_controller_positions, vector<core_id_t>& core_list_from_cfg_file, SInt32 application_core_count);

   protected:
      Network* getNetwork() { return m_network; }
      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

      vector<core_id_t> getCoreListWithMemoryControllers(void);
      void printCoreListWithMemoryControllers(vector<core_id_t>& core_list_with_memory_controllers);
   
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

      virtual core_id_t getShmemRequester(const void* pkt_data) = 0;

      virtual void enableModels() = 0;
      virtual void disableModels() = 0;

      Core* getCore() { return m_core; }
      
      static CachingProtocol_t parseProtocolType(std::string& protocol_type);
      static MemoryManagerBase* createMMU(std::string protocol_type,
            Core* core,
            Network* network, 
            ShmemPerfModel* shmem_perf_model);
      
      virtual void outputSummary(std::ostream& os) = 0;
};

#endif /* __MEMORY_MANAGER_BASE_H__ */
