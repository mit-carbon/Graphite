#ifndef PERFORMANCE_MODEL_H
#define PERFORMANCE_MODEL_H
// This class represents the actual performance model for a given core
#include "instruction.h"
#include "fixed_types.h"

class PerformanceModel
{
public:
   PerformanceModel();
   ~PerformanceModel();
   void handleInstruction(Instruction *instruction);

   UInt64 getInstructionCount() { return m_instruction_count; }
   UInt64 getCycleCount() { return m_cycle_count; }

private:
   UInt64 m_instruction_count;
   UInt64 m_cycle_count;
};

#endif
