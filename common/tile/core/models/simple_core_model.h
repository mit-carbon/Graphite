#ifndef SIMPLE_CORE_MODEL_H
#define SIMPLE_CORE_MODEL_H

#include "core_model.h"

class SimpleCoreModel : public CoreModel
{
public:
   SimpleCoreModel(Core *core, float frequency);
   ~SimpleCoreModel();

   void updateInternalVariablesOnFrequencyChange(volatile float frequency);
   void reset();
   void outputSummary(std::ostream &os);

private:
   void handleInstruction(Instruction *instruction);
   
   UInt64 modelICache(IntPtr ins_address, UInt32 ins_size);
   void initializePipelineStallCounters();

   UInt64 m_total_l1icache_stall_cycles;
   UInt64 m_total_l1dcache_read_stall_cycles;
   UInt64 m_total_l1dcache_write_stall_cycles;
};

#endif
