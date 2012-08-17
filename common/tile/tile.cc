#include "tile.h"
#include "tile_manager.h"
#include "network.h"
#include "network_model.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "core_model.h"
#include "main_core.h"
#include "core.h"
#include "simulator.h"
#include "log.h"

using namespace std;

Tile::Tile(tile_id_t id)
   : m_tile_id(id)
   , m_shmem_perf_model(NULL)
   , m_memory_manager(NULL)
{
   LOG_PRINT("Tile ctor for: %d", id);

   m_network = new Network(this);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_shmem_perf_model = new ShmemPerfModel();
      LOG_PRINT("instantiated shared memory performance model");

      m_memory_manager = MemoryManager::createMMU(Sim()->getCfg()->getString("caching_protocol/type"),
                                                  this, this->getNetwork(), m_shmem_perf_model);
      LOG_PRINT("instantiated memory manager model");
   }

   m_main_core = new MainCore(this);
}

Tile::~Tile()
{
   delete m_main_core;
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      delete m_memory_manager;
      delete m_shmem_perf_model;
   }
   delete m_network;
}

void Tile::outputSummary(std::ostream &os)
{
   os << "Tile Summary:\n";
   LOG_PRINT("Core Performance Model Summary");
   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      getCore()->getPerformanceModel()->outputSummary(os);
   }
   LOG_PRINT("Network Summary");
   getNetwork()->outputSummary(os);

   LOG_PRINT("Memory Model Summary");
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      getCore()->getShmemPerfModel()->outputSummary(os, Config::getSingleton()->getCoreFrequency(getCore()->getId()));
      getCore()->getMemoryManager()->outputSummary(os);
   }
}

void Tile::enablePerformanceModels()
{
   LOG_PRINT("enablePerformanceModels(%i) start", m_tile_id);
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->enable();

   getNetwork()->enableModels();
   getCore()->getShmemPerfModel()->enable();
   getCore()->getMemoryManager()->enableModels();
   getCore()->getPerformanceModel()->enable();
   LOG_PRINT("enablePerformanceModels(%i) end", m_tile_id);
}

void Tile::disablePerformanceModels()
{
   LOG_PRINT("disablePerformanceModels(%i) start", m_tile_id);
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->disable();

   getNetwork()->disableModels();
   getCore()->getShmemPerfModel()->disable();
   getCore()->getMemoryManager()->disableModels();
   getCore()->getPerformanceModel()->disable();
   LOG_PRINT("disablePerformanceModels(%i) end", m_tile_id);
}

void
Tile::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   getCore()->getPerformanceModel()->updateInternalVariablesOnFrequencyChange(frequency);
   getCore()->getShmemPerfModel()->updateInternalVariablesOnFrequencyChange(frequency);
}
