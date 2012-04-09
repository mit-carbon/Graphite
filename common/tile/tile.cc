#include "tile.h"
#include "tile_manager.h"
#include "network.h"
#include "network_model.h"
#include "syscall_model.h"
#include "sync_client.h"
#include "network_types.h"
#include "memory_manager_base.h"
#include "pin_memory_manager.h"
#include "clock_skew_minimization_object.h"
#include "core_model.h"
#include "core.h"
#include "simulator.h"
#include "log.h"

using namespace std;

Tile::Tile(SInt32 id)
   : m_tile_id(id)
{
   LOG_PRINT("Tile ctor for: %d", id);

   m_network = new Network(this);

   // Initialize memory models as NULL, the Core constructor will create them.
   //m_shmem_perf_model = (ShmemPerfModel*) NULL;
   //m_memory_manager = (MemoryManagerBase *) NULL;

   m_shmem_perf_model = new ShmemPerfModel();
   LOG_PRINT("instantiated shared memory performance model");

   m_memory_manager = MemoryManagerBase::createMMU(
         Sim()->getCfg()->getString("caching_protocol/type"),
         this, this->getNetwork(), m_shmem_perf_model);
   LOG_PRINT("instantiated memory manager model");

   m_main_core = Core::create(this, MAIN_CORE_TYPE);
}

Tile::~Tile()
{
   LOG_PRINT("Deleting tile with id %d", this->getId());
   delete m_main_core;
}

void Tile::outputSummary(std::ostream &os)
{
   os << "Tile Summary:\n";
   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      getCore()->getPerformanceModel()->outputSummary(os);
   }
   getNetwork()->outputSummary(os);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      getCore()->getShmemPerfModel()->outputSummary(os, Config::getSingleton()->getCoreFrequency(getCore()->getCoreId()));
      getCore()->getMemoryManager()->outputSummary(os);
   }
}

void Tile::enablePerformanceModels()
{
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->enable();

   getNetwork()->enableModels();

   getCore()->getShmemPerfModel()->enable();
   getCore()->getMemoryManager()->enableModels();
   getCore()->getPerformanceModel()->enable();
}

void Tile::disablePerformanceModels()
{
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->disable();

   getNetwork()->disableModels();

   getCore()->getShmemPerfModel()->disable();
   getCore()->getMemoryManager()->disableModels();
   getCore()->getPerformanceModel()->disable();
}

void Tile::resetPerformanceModels()
{
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->reset();

   getCore()->getShmemPerfModel()->reset();
   getCore()->getMemoryManager()->resetModels();
   getCore()->getNetwork()->resetModels();
   getCore()->getPerformanceModel()->reset();
}

void
Tile::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   getCore()->getPerformanceModel()->updateInternalVariablesOnFrequencyChange(frequency);
   getCore()->getShmemPerfModel()->updateInternalVariablesOnFrequencyChange(frequency);
}

Core* Tile::getCore(core_id_t core_id)
{
   Core * res = NULL;

   if (this->isMainCore(core_id))
      res = m_main_core;

   LOG_ASSERT_ERROR(res != NULL, "Invalid core id!");
   return res;
}

// This method is used for differentiating different cores if you decide to add different types of cores per tile.
Core* Tile::getCurrentCore()
{
   return getCore();
}

core_id_t Tile::getMainCoreId()
{
   return (core_id_t) {m_tile_id, MAIN_CORE_TYPE};
}
bool Tile::isMainCore(core_id_t core_id)
{
   return (core_id.core_type == MAIN_CORE_TYPE);
}

