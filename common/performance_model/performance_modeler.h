#ifndef PERFORMANCE_MODELER_H
#define PERFORMANCE_MODELER_H
// This class provides the interface to performance modeling to the
// front end. It uses a queue of instructions to do the performance
// modeling.
#include "instruction.h"

class PerformanceModeler
{
public:
   PerformanceModeler();
   ~PerformanceModeler();
   void handleInstruction(Instruction *instruction);
};

#endif
