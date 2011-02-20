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
#include "core_perf_model.h"
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
   m_shmem_perf_model = (ShmemPerfModel*) NULL;
   m_memory_manager = (MemoryManagerBase *) NULL;

   m_main_core = Core::create(this, Core::MAIN_CORE_TYPE);
   if(Config::getSingleton()->getEnablePepCores())
      m_pep_core = Core::create(this, Core::PEP_CORE_TYPE);
   else
      m_pep_core = NULL;
}

Tile::~Tile()
{

   LOG_PRINT("Deleting tile with id %d", this->getId());
   delete m_main_core;
   delete m_pep_core;
}

void Tile::outputSummary(std::ostream &os)
{
   os << "Core summary:\n";
   if (Config::getSingleton()->getEnablePerformanceModeling())
   {
      if (!Config::getSingleton()->getEnablePepCores())
      {
         getCore()->getPerformanceModel()->outputSummary(os);
      }
      else
      {
         os << " Main core:\n";
         getCore()->getPerformanceModel()->outputSummary(os);
         os << " PEP core:\n";
         getPepCore()->getPerformanceModel()->outputSummary(os);
      }
   }
   getNetwork()->outputSummary(os);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      getCore()->getShmemPerfModel()->outputSummary(os, Config::getSingleton()->getCoreFrequency(getId()));
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

   if (Config::getSingleton()->getEnablePepCores())
   {
      if (getPepCore()->getClockSkewMinimizationClient())
         getPepCore()->getClockSkewMinimizationClient()->enable();

      getPepCore()->getShmemPerfModel()->enable();
      getPepCore()->getMemoryManager()->enableModels();
      getPepCore()->getPerformanceModel()->enable();
   }
}

void Tile::disablePerformanceModels()
{
   if (getCore()->getClockSkewMinimizationClient())
      getCore()->getClockSkewMinimizationClient()->disable();

   getNetwork()->disableModels();

   getCore()->getShmemPerfModel()->disable();
   getCore()->getMemoryManager()->disableModels();
   getCore()->getPerformanceModel()->disable();

   if (Config::getSingleton()->getEnablePepCores())
   {
      if (getPepCore()->getClockSkewMinimizationClient())
         getPepCore()->getClockSkewMinimizationClient()->disable();
         
      getPepCore()->getShmemPerfModel()->disable();
      getPepCore()->getMemoryManager()->disableModels();
      getPepCore()->getPerformanceModel()->disable();
   }
}

void
Tile::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   getCore()->getPerformanceModel()->updateInternalVariablesOnFrequencyChange(frequency);
   getPepCore()->getPerformanceModel()->updateInternalVariablesOnFrequencyChange(frequency);

   getCore()->getShmemPerfModel()->updateInternalVariablesOnFrequencyChange(frequency);
   getPepCore()->getShmemPerfModel()->updateInternalVariablesOnFrequencyChange(frequency);

   getCore()->getMemoryManager()->updateInternalVariablesOnFrequencyChange(frequency);
   getPepCore()->getMemoryManager()->updateInternalVariablesOnFrequencyChange(frequency);
}

Core* Tile::getCore(core_id_t core_id)
{
   Core * res = NULL;
   if (core_id.second == MAIN_CORE_TYPE)
      res = m_main_core;
   else if (core_id.second == PEP_CORE_TYPE)
      res = m_pep_core;

   LOG_ASSERT_ERROR(res != NULL, "Invalid core id!");
   return res;
}

Core* Tile::getCurrentCore()
{
   // elau: Tile's getting the manager?  Fix this...
   return Sim()->getTileManager()->getCore(this);
}
