#ifndef TILE_H
#define TILE_H

#include <string.h>

// some forward declarations for cross includes
//class Network;
//class MemoryManagerBase;
//class SyscallMdl;
//class SyncClient;
//class ClockSkewMinimizationClient;
class CorePerfModel;

// FIXME: Move this out of here eventually
class PinMemoryManager;

//#include "mem_component.h"
#include "fixed_types.h"
#include "config.h"
#include "core_perf_model.h"
//#include "shmem_perf_model.h"
#include "capi.h"
#include "packet_type.h"
#include "tile.h"

using namespace std;

class Core
{
   public:

      enum core_type_t
      {
         MAIN_CORE_TYPE = 0,
         PEP_CORE_TYPE
      };

      Core(SInt32 id);
      ~Core();

      static Core *create(Tile* tile, core_type_t core_type = MAIN_CORE_TYPE);

      //int coreSendW(int sender, int receiver, char *buffer, int size, carbon_network_t net_type);
      //int coreRecvW(int sender, int receiver, char *buffer, int size, carbon_network_t net_type);
     
      //UInt64 readInstructionMemory(IntPtr address, 
            //UInt32 instruction_size);

      //pair<UInt32, UInt64> initiateMemoryAccess(
            //MemComponent::component_t mem_component,
            //lock_signal_t lock_signal, 
            //mem_op_t mem_op_type, 
            //IntPtr address, 
            //Byte* data_buf, UInt32 data_size,
            //bool modeled = false,
            //UInt64 time = 0);
      
      //pair<UInt32, UInt64> accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool modeled = false);
      //pair<UInt32, UInt64> nativeMemOp(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size);

      // network accessor since network is private
      int getId() { return m_core_id; }
      //Network *getNetwork() { return m_network; }
      CorePerfModel *getPerformanceModel() { return m_core_perf_model; }
      //MemoryManagerBase *getMemoryManager() { return m_memory_manager; }
      //PinMemoryManager *getPinMemoryManager() { return m_pin_memory_manager; }
      //SyscallMdl *getSyscallMdl() { return m_syscall_model; }
      //SyncClient *getSyncClient() { return m_sync_client; }
      //ClockSkewMinimizationClient* getClockSkewMinimizationClient() { return m_clock_skew_minimization_client; }
      //ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

      //void updateInternalVariablesOnFrequencyChange(volatile float frequency);

      //State getState();
      //void setState(State tile_state);

      //void enablePerformanceModels();
      //void disablePerformanceModels();

   private:
      core_id_t m_core_id;
      //MemoryManagerBase *m_memory_manager;
      //PinMemoryManager *m_pin_memory_manager;
      //Network *m_network;
      CorePerfModel *m_core_perf_model;
      //CorePerfModel *m_pep_core_perf_model;
      //SyscallMdl *m_syscall_model;
      //SyncClient *m_sync_client;
      //ClockSkewMinimizationClient *m_clock_skew_minimization_client;
      //ShmemPerfModel* m_shmem_perf_model;
      
      State m_core_state;
      Lock m_core_state_lock;

      PacketType getPktTypeFromUserNetType(carbon_network_t net_type);
};

#endif
