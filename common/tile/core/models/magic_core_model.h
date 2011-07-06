#ifndef MAGIC_CORE_MODEL_H
#define MAGIC_CORE_MODEL_H

#include "core_model.h"

class MagicCoreModel : public CoreModel
{
public:
   MagicCoreModel(Core *core, float frequency);
   ~MagicCoreModel();

   void reset();
   void outputSummary(std::ostream &os);

   UInt64 getInstructionCount() { return m_instruction_count; }

private:
   void handleInstruction(Instruction *instruction);

   bool isModeled(InstructionType instruction_type);
   
   UInt64 m_instruction_count;
};

#endif
