#include "simulator.h"
#include "log.h"
#include "performance_modeler.h"
#include "core_manager.h"


PerformanceModeler::PerformanceModeler()
{
   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();
   m_performance_models = new PerformanceModel* [num_local_cores];
   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      m_performance_models[i] = new SimplePerformanceModel();
   }
   Instruction::initializeStaticInstructionModel();
}

PerformanceModeler::~PerformanceModeler()
{
   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();

   for (UInt32 i = 0; i < num_local_cores; i++)
   {
      delete m_performance_models[i];
   }
   
   delete [] m_performance_models;
   m_performance_models = NULL;
}

void PerformanceModeler::outputSummary(std::ostream &os)
{
   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();
   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   os << "Model Summary:\n";
   for(unsigned int i = 0; i < num_local_cores; i++)
   {
      os << "Core: " << local_cores[i] << endl;
      m_performance_models[i]->outputSummary(os);
   }
}

PerformanceModel* PerformanceModeler::getPerformanceModel()
{
   LOG_ASSERT_ERROR(m_performance_models, "Performance model accessed after deletion.");
   UInt32 core_index = Sim()->getCoreManager()->getCoreIndexFromTID(Sim()->getCoreManager()->getCurrentTID());
   return m_performance_models[core_index];
}

PerformanceModel* PerformanceModeler::getPerformanceModelForCore(core_id_t core_id)
{
   LOG_ASSERT_ERROR(m_performance_models, "Performance model accessed after deletion.");
   UInt32 core_index = Sim()->getCoreManager()->getCoreIndexFromID(core_id);
   return m_performance_models[core_index];
}

