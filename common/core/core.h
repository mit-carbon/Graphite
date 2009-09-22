#ifndef CORE_H
#define CORE_H

#include <string>

// some forward declarations for cross includes
class Network;
class MemoryManagerBase;
class SyscallMdl;
class SyncClient;
class PerformanceModel;

// FIXME: Move this out of here eventually
class PinMemoryManager;

#include "mem_component.h"
#include "fixed_types.h"
#include "config.h"
#include "performance_model.h"
#include "shmem_perf_model.h"

#define REDIRECT_MEMORY 1

using namespace std;

class Core
{
   public:

      enum lock_signal_t
      {
         INVALID_LOCK_SIGNAL = 0,
         MIN_LOCK_SIGNAL,
         NONE = MIN_LOCK_SIGNAL,
         LOCK,
         UNLOCK,
         MAX_LOCK_SIGNAL = UNLOCK,
         NUM_LOCK_SIGNAL_TYPES = MAX_LOCK_SIGNAL - MIN_LOCK_SIGNAL + 1
      };
     
      enum mem_op_t
      {
         INVALID_MEM_OP = 0,
         MIN_MEM_OP,
         READ = MIN_MEM_OP,
         READ_EX,
         WRITE,
         MAX_MEM_OP = WRITE,
         NUM_MEM_OP_TYPES = MAX_MEM_OP - MIN_MEM_OP + 1
      };

      Core(SInt32 id);
      ~Core();

      void outputSummary(std::ostream &os);

      int coreSendW(int sender, int receiver, char *buffer, int size);
      int coreRecvW(int sender, int receiver, char *buffer, int size);
     
      UInt64 readInstructionMemory(IntPtr address, 
            UInt32 instruction_size);

      pair<UInt32, UInt64> initiateMemoryAccess(
            MemComponent::component_t mem_component,
            lock_signal_t lock_signal, 
            mem_op_t mem_op_type, 
            IntPtr address, 
            Byte* data_buf, UInt32 data_size,
            bool modeled = false);
      
      pair<UInt32, UInt64> accessMemory(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size, bool modeled = false);
      pair<UInt32, UInt64> nativeMemOp(lock_signal_t lock_signal, mem_op_t mem_op_type, IntPtr d_addr, char* data_buffer, UInt32 data_size);


      // network accessor since network is private
      int getId() { return m_core_id; }
      Network *getNetwork() { return m_network; }
      PerformanceModel *getPerformanceModel() { return m_performance_model; }
      MemoryManagerBase *getMemoryManager() { return m_memory_manager; }
      PinMemoryManager *getPinMemoryManager() { return m_pin_memory_manager; }
      SyscallMdl *getSyscallMdl() { return m_syscall_model; }
      SyncClient *getSyncClient() { return m_sync_client; }
      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

      void enablePerformanceModels();
      void disablePerformanceModels();

   private:
      core_id_t m_core_id;
      MemoryManagerBase *m_memory_manager;
      PinMemoryManager *m_pin_memory_manager;
      Network *m_network;
      PerformanceModel *m_performance_model;
      SyscallMdl *m_syscall_model;
      SyncClient *m_sync_client;
      ShmemPerfModel* m_shmem_perf_model;

      static Lock m_global_core_lock;
};

#endif
