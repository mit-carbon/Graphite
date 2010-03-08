#ifndef MAGIC_PERFORMANCE_MODEL_H
#define MAGIC_PERFORMANCE_MODEL_H

#include "performance_model.h"

class MagicPerformanceModel : public PerformanceModel
{
public:
   MagicPerformanceModel(Core *core);
   ~MagicPerformanceModel();

   void outputSummary(std::ostream &os);

   UInt64 getInstructionCount() { return m_instruction_count; }
   UInt64 getCycleCount() { return m_cycle_count; }
   void resetCycleCount() { m_cycle_count = (UInt64) 0; }

protected:
   void setCycleCount(UInt64 time);

private:
   void handleInstruction(Instruction *instruction);

   bool isModeled(InstructionType instruction_type);
   
   UInt64 m_instruction_count;
   UInt64 m_cycle_count;
};

#endif
