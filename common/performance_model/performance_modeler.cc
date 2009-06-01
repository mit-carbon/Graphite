#include "simulator.h"
#include "performance_modeler.h"
#include "core_manager.h"

PerformanceModeler::PerformanceModeler()
{
   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();
   m_performance_models = new PerformanceModel[num_local_cores];
}

PerformanceModeler::~PerformanceModeler()
{
   UInt32 num_local_cores = Config::getSingleton()->getNumLocalCores();
   UInt32 proc_id = Config::getSingleton()->getCurrentProcessNum();
   const Config::CoreList &local_cores = Config::getSingleton()->getCoreListForProcess(proc_id);

   fprintf(stderr, "Model Summary:\n");
   for(unsigned int i = 0; i < num_local_cores; i++)
   {
       fprintf(stderr, "Core: %d\n", local_cores[i]);
       fprintf(stderr, "  Instructions: %lld\n", m_performance_models[i].getInstructionCount());
       fprintf(stderr, "  Cycles: %lld\n", m_performance_models[i].getCycleCount());
   }


    delete [] m_performance_models;
    m_performance_models = NULL;
}


PerformanceModel* PerformanceModeler::getPerformanceModel()
{
   UInt32 core_index = Sim()->getCoreManager()->getCoreIndexFromTID(Sim()->getCoreManager()->getCurrentTID());
   return &m_performance_models[core_index];
}

PerformanceModel* PerformanceModeler::getPerformanceModelForCore(core_id_t core_id)
{
   UInt32 core_index = Sim()->getCoreManager()->getCoreIndexFromID(core_id);
   return &m_performance_models[core_index];
}

