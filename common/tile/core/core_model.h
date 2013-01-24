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
#include "lock.h"
#include "dynamic_instruction_info.h"

class CoreModel
{
public:

   CoreModel(Core* core, float frequency);
   virtual ~CoreModel();

   void queueDynamicInstruction(Instruction *i);
   void queueBasicBlock(BasicBlock *basic_block);
   void iterate();

   volatile float getFrequency() { return m_frequency; }
   virtual void updateInternalVariablesOnFrequencyChange(volatile float frequency);
   void recomputeAverageFrequency(); 

   UInt64 getCycleCount() { return m_cycle_count; }
   void setCycleCount(UInt64 cycle_count);

   void pushDynamicInstructionInfo(DynamicInstructionInfo &i);
   void popDynamicInstructionInfo();
   DynamicInstructionInfo& getDynamicInstructionInfo();

   static CoreModel *create(Core* core);

   BranchPredictor *getBranchPredictor() { return m_bp; }

   void enable();
   void disable();
   bool isEnabled() { return m_enabled; }

   virtual void outputSummary(std::ostream &os) = 0;

   virtual void computeEnergy() = 0;

   virtual double getDynamicEnergy() { return 0; }
   virtual double getStaticPower()   { return 0; }

   class AbortInstructionException { };


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
   typedef std::queue<BasicBlock *> BasicBlockQueue;

   Core* getCore() { return m_core; }

   UInt64 m_cycle_count;
   UInt64 m_instruction_count;
   
   volatile float m_frequency;

   void updatePipelineStallCounters(Instruction* i, UInt64 memory_stall_cycles, UInt64 execution_unit_stall_cycles);

private:

   class DynamicInstructionInfoNotAvailableException { };

   virtual void handleInstruction(Instruction *instruction) = 0;

   // Pipeline Stall Counters
   void initializePipelineStallCounters();

   Core* m_core;

   volatile float m_average_frequency;
   UInt64 m_total_time;
   UInt64 m_checkpointed_cycle_count;

   bool m_enabled;

   BasicBlockQueue m_basic_block_queue;
   Lock m_basic_block_queue_lock;

   DynamicInstructionInfoQueue m_dynamic_info_queue;
   Lock m_dynamic_info_queue_lock;

   UInt32 m_current_ins_index;

   BranchPredictor *m_bp;

   // Pipeline Stall Counters
   UInt64 m_total_recv_instructions;
   UInt64 m_total_sync_instructions;
   UInt64 m_total_recv_instruction_stall_cycles;
   UInt64 m_total_sync_instruction_stall_cycles;
   UInt64 m_total_memory_stall_cycles;
   UInt64 m_total_execution_unit_stall_cycles;
};

#endif
