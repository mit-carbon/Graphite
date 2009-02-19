// Harshad Kasture
//

#ifndef CORE_H
#define CORE_H

#include <iostream>
#include <fstream>
#include <string.h>

// some forward declarations for cross includes
class Network;
class MemoryManager;
class SyscallMdl;
class SyncClient;
class OCache;
class PerfModel;
class Network;

#include "fixed_types.h"
#include "config.h"

#include "cache.h"
#include "cache_state.h"
#include "dram_directory_entry.h"
#include "perfmdl.h"

class Core
{
   public:
      Core(SInt32 id);
      ~Core();

      int coreSendW(int sender, int receiver, char *buffer, int size);
      int coreRecvW(int sender, int receiver, char *buffer, int size);

      // network accessor since network is private
      int getId() { return m_core_id; }
      Network *getNetwork() { return m_network; }
      PerfModel *getPerfModel() { return m_perf_model; }
      MemoryManager *getMemoryManager() { return m_memory_manager; }
      SyscallMdl *getSyscallMdl() { return m_syscall_model; }
      SyncClient *getSyncClient() { return m_sync_client; }
      OCache *getOCache() { return m_ocache; }

   private:
      SInt32 m_core_id;
      MemoryManager *m_memory_manager;
      Network *m_network;
      PerfModel *m_perf_model;
      OCache *m_ocache;
      SyscallMdl *m_syscall_model;
      SyncClient *m_sync_client;
};

#endif
