#ifndef PERFORMANCE_MODEL_H
#define PERFORMANCE_MODEL_H
// This class represents the actual performance model for a given core

#include <queue>
#include <iostream>

#include "instruction.h"
#include "basic_block.h"
#include "fixed_types.h"
#include "lock.h"
#include "dynamic_instruction_info.h"

class PerformanceModel
{
public:
   PerformanceModel();
   virtual ~PerformanceModel();

   void queueDynamicInstruction(Instruction *i);
   void queueBasicBlock(BasicBlock *basic_block);
   void iterate();

   virtual void outputSummary(std::ostream &os) = 0;

   virtual UInt64 getCycleCount() = 0;

   void pushDynamicInstructionInfo(DynamicInstructionInfo &i);
   void popDynamicInstructionInfo();
   DynamicInstructionInfo& getDynamicInstructionInfo();

   class DynamicInstructionInfoNotAvailable { };

private:
   virtual void handleInstruction(Instruction *instruction) = 0;

   typedef std::queue<BasicBlock *> BasicBlockQueue;
   BasicBlockQueue m_basic_block_queue;
   Lock m_basic_block_queue_lock;

   typedef std::queue<DynamicInstructionInfo> DynamicInstructionInfoQueue;
   DynamicInstructionInfoQueue m_dynamic_info_queue;
   Lock m_dynamic_info_queue_lock;

   UInt32 m_current_ins_index;
};

class SimplePerformanceModel : public PerformanceModel
{
public:
   SimplePerformanceModel();
   ~SimplePerformanceModel();

   void outputSummary(std::ostream &os);

   UInt64 getInstructionCount() { return m_instruction_count; }
   UInt64 getCycleCount() { return m_cycle_count; }

private:
   void handleInstruction(Instruction *instruction);
   
   UInt64 m_instruction_count;
   UInt64 m_cycle_count;
};

#endif
