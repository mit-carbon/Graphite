#ifndef SIMPLE_CORE_MODEL_H
#define SIMPLE_CORE_MODEL_H

#include "core_model.h"

class SimpleCoreModel : public CoreModel
{
public:
   SimpleCoreModel(Core *core);
   ~SimpleCoreModel();

   void outputSummary(std::ostream &os);

private:
   void handleInstruction(Instruction *instruction);
   
   Time modelICache(IntPtr ins_address, UInt32 ins_size);
   void initializePipelineStallCounters();

   Time m_total_l1icache_stall_time;
   Time m_total_l1dcache_read_stall_time;
   Time m_total_l1dcache_write_stall_time;
};

#endif
