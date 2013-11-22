#ifndef SIMPLE_CORE_MODEL_H
#define SIMPLE_CORE_MODEL_H

#include "core_model.h"

class SimpleCoreModel : public CoreModel
{
public:
   SimpleCoreModel(Core *core);
   ~SimpleCoreModel();

   void outputSummary(std::ostream &os, const Time& target_completion_time);

private:
   void handleInstruction(Instruction *instruction);
   
   void initializePipelineStallCounters();

   Time _total_l1icache_stall_time;
   Time _total_l1dcache_read_stall_time;
   Time _total_l1dcache_write_stall_time;
};

#endif
