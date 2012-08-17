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
   Tile(tile_id_t id);
   ~Tile();

   void outputSummary(std::ostream &os);

   // network accessor since network is private
   int getId()                         { return m_tile_id; }
   Network *getNetwork()               { return m_network; }
   Core* getCore()                     { return m_main_core; }
   MemoryManager *getMemoryManager()   { return m_memory_manager; }
   ShmemPerfModel* getShmemPerfModel() { return m_shmem_perf_model; }

   static core_id_t getMainCoreId(tile_id_t id)    { return (core_id_t) {id, MAIN_CORE_TYPE}; }
   static bool isMainCore(core_id_t core_id)       { return (core_id.core_type == MAIN_CORE_TYPE); }

   void updateInternalVariablesOnFrequencyChange(volatile float frequency);

   void enablePerformanceModels();
   void disablePerformanceModels();

private:
   tile_id_t m_tile_id;
   Network *m_network;
   ShmemPerfModel* m_shmem_perf_model;
   MemoryManager *m_memory_manager;
   Core *m_main_core;
};

#endif
