#ifndef TILE_H
#define TILE_H

#include <string.h>

// some forward declarations for cross includes
//class Network;
class MemoryManager;
class SyscallMdl;
class SyncClient;
class ClockSkewMinimizationClient;

#include "mem_component.h"
#include "fixed_types.h"
#include "config.h"
#include "core.h"
#include "packet_type.h"
#include "shmem_perf_model.h"
#include "capi.h"
#include "packet_type.h"

using namespace std;

class Tile
{
   public:
      Tile(SInt32 id);
      ~Tile();

      void outputSummary(std::ostream &os);

      // network accessor since network is private
      int getId() { return m_tile_id; }
      Network *getNetwork() { return m_network; }
      Core* getCore(core_id_t core_id);
      Core* getCurrentCore();
      Core* getCore() {return m_main_core; }
      MemoryManager *getMemoryManager() { return m_memory_manager; }
      ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

      core_id_t getMainCoreId();
      bool isMainCore(core_id_t core_id);

      void setMemoryManager(MemoryManager *memory_manager) { m_memory_manager = memory_manager; }
      void setShmemPerfModel(ShmemPerfModel *shmem_perf_model) { m_shmem_perf_model = shmem_perf_model; }

      void updateInternalVariablesOnFrequencyChange(volatile float frequency);

      void enablePerformanceModels();
      void disablePerformanceModels();

   private:
      tile_id_t m_tile_id;
      MemoryManager *m_memory_manager;
      Network *m_network;
      Core *m_main_core;
      ShmemPerfModel* m_shmem_perf_model;
};

#endif
