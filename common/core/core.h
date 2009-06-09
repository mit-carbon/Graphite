#ifndef CORE_H
#define CORE_H

#include <string>

// some forward declarations for cross includes
class Network;
class PerfModel;
class MemoryManager;
class SyscallMdl;
class SyncClient;
class Cache;

#include "shmem_req_types.h"
#include "fixed_types.h"
#include "config.h"
#include "perfmdl.h"

#define REDIRECT_MEMORY 1

using namespace std;

class Core
{
   public:
     
      enum lock_signal_t
      {
         NONE = 0,
         LOCK,
         UNLOCK,
         NUM_LOCK_SIGNALS
      };

      Core(SInt32 id);
      ~Core();

      int coreSendW(int sender, int receiver, char *buffer, int size);
      int coreRecvW(int sender, int receiver, char *buffer, int size);
      UInt32 accessMemory(lock_signal_t lock_signal, shmem_req_t shmem_req_type, IntPtr d_addr, char* data_buffer, UInt32 data_size);
      UInt32 nativeMemOp(lock_signal_t lock_signal, shmem_req_t shmem_req_type, IntPtr d_addr, char* data_buffer, UInt32 data_size);

      // network accessor since network is private
      int getId() { return m_core_id; }
      Network *getNetwork() { return m_network; }
      PerfModel *getPerfModel() { return m_perf_model; }
      MemoryManager *getMemoryManager() { return m_memory_manager; }
      SyscallMdl *getSyscallMdl() { return m_syscall_model; }
      SyncClient *getSyncClient() { return m_sync_client; }
      Cache *getOCache() { return m_ocache; }

   private:
      core_id_t m_core_id;
      MemoryManager *m_memory_manager;
      Network *m_network;
      PerfModel *m_perf_model;
      Cache *m_ocache;
      SyscallMdl *m_syscall_model;
      SyncClient *m_sync_client;

      UInt32 m_cache_line_size;

      static Lock m_global_core_lock;
};

#endif
