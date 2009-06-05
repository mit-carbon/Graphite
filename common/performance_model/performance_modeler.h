#ifndef PERFORMANCE_MODELER_H
#define PERFORMANCE_MODELER_H
// This class provides the interface to performance modeling to the
// front end. It uses a queue of instructions to do the performance
// modeling.
#include <map>
#include <iostream>
#include "instruction.h"
#include "performance_model.h"

class PerformanceModeler
{
public:
   PerformanceModeler();
   ~PerformanceModeler();

   PerformanceModel* getPerformanceModel();
   PerformanceModel* getPerformanceModelForCore(core_id_t core_id);

   void outputSummary(std::ostream &os);

private:

   // The performance modeler has one model per core.
   PerformanceModel **m_performance_models;
};

#endif
