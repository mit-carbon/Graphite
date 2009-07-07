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

class BranchPredictor;

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

   static PerformanceModel *create();

   BranchPredictor *getBranchPredictor() { return m_bp; }

protected:
   friend class SpawnInstruction;

   virtual void setCycleCount(UInt64 time) = 0;

   typedef std::queue<DynamicInstructionInfo> DynamicInstructionInfoQueue;
   typedef std::queue<BasicBlock *> BasicBlockQueue;

private:

   virtual void handleInstruction(Instruction *instruction) = 0;

   BasicBlockQueue m_basic_block_queue;
   Lock m_basic_block_queue_lock;

   DynamicInstructionInfoQueue m_dynamic_info_queue;
   Lock m_dynamic_info_queue_lock;

   UInt32 m_current_ins_index;

   BranchPredictor *m_bp;
};

#endif
