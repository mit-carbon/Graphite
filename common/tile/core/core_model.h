#ifndef CORE_MODEL_H
#define CORE_MODEL_H
// This class represents the actual performance model for a given core

#include <queue>
#include <iostream>
#include <string>

// Forward Decls
class Core;
class BranchPredictor;

#include "instruction.h"
#include "basic_block.h"
#include "fixed_types.h"
#include "dynamic_instruction_info.h"
#include "time_types.h"

class CoreModel
{
public:
   CoreModel(Core* core);
   virtual ~CoreModel();

   void processDynamicInstruction(DynamicInstruction* i);
   void queueInstruction(Instruction* instruction);
   void iterate();

   virtual void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);
   void recomputeAverageFrequency(float frequency); 

   Time getCurrTime(){return m_curr_time; };
   void setCurrTime(Time time);

   void pushDynamicInstructionInfo(DynamicInstructionInfo &i);
   void popDynamicInstructionInfo();
   DynamicInstructionInfo& getDynamicInstructionInfo();

   static CoreModel* create(Core* core);

   BranchPredictor *getBranchPredictor() { return m_bp; }

   void enable();
   void disable();
   bool isEnabled() { return m_enabled; }

   virtual void outputSummary(std::ostream &os) = 0;

   class AbortInstructionException { };

   Time getCost(InstructionType type);

   Core* getCore(){return m_core;};

protected:
   enum RegType
   {
      INTEGER = 0,
      FLOATING_POINT
   };
   enum AccessType
   {
      READ = 0,
      WRITE
   };
   enum ExecutionUnitType
   {
   };

   friend class SpawnInstruction;

   typedef std::queue<DynamicInstructionInfo> DynamicInstructionInfoQueue;
   typedef std::queue<Instruction*> InstructionQueue;

   Core* m_core;

   Time m_curr_time;
   UInt64 m_instruction_count;
   
   void updatePipelineStallCounters(Instruction* i, Time memory_stall_time, Time execution_unit_stall_time);

   void updateCoreStaticInstructionModel(volatile float frequency);

private:

   class DynamicInstructionInfoNotAvailableException { };

   virtual void handleInstruction(Instruction *instruction) = 0;

   // Pipeline Stall Counters
   void initializePipelineStallCounters();

   volatile float m_average_frequency;
   Time m_total_time;
   Time m_checkpointed_time;
   UInt64 m_total_cycles;

   bool m_enabled;

   InstructionQueue m_instruction_queue;
   DynamicInstructionInfoQueue m_dynamic_info_queue;

   BranchPredictor *m_bp;

   // Instruction costs
   typedef std::vector<Time> CoreStaticInstructionCosts;
   CoreStaticInstructionCosts m_core_instruction_costs;
   void initializeCoreStaticInstructionModel(volatile float frequency);

   // Pipeline Stall Counters
   UInt64 m_total_recv_instructions;
   UInt64 m_total_sync_instructions;
   Time m_total_recv_instruction_stall_time;
   Time m_total_sync_instruction_stall_time;
   Time m_total_memory_stall_time;
   Time m_total_execution_unit_stall_time;
};

#endif
