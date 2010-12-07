#ifndef MAGIC_PEP_PERFORMANCE_MODEL_H
#define MAGIC_PEP_PERFORMANCE_MODEL_H

#include "core_perf_model.h"

class MagicPepPerformanceModel : public CorePerfModel
{
public:
   MagicPepPerformanceModel(Core *core, float frequency);
   ~MagicPepPerformanceModel();

   void outputSummary(std::ostream &os);

   UInt64 getInstructionCount() { return m_instruction_count; }

private:
   void handleInstruction(Instruction *instruction);

   bool isModeled(InstructionType instruction_type);
   
   UInt64 m_instruction_count;
};

#endif
