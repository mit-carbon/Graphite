#ifndef PERFORMANCE_MODEL_H
#define PERFORMANCE_MODEL_H
// This class represents the actual performance model for a given core

#include <queue>

#include "instruction.h"
#include "fixed_types.h"

typedef std::queue<BasicBlock *> BasicBlockQueue;

class PerformanceModel
{
public:
   PerformanceModel();
   ~PerformanceModel();

   void queueBasicBlock(BasicBlock *basic_block);
   void iterate();

   UInt64 getInstructionCount() { return m_instruction_count; }
   UInt64 getCycleCount() { return m_cycle_count; }

private:
   void handleInstruction(Instruction *instruction);


   BasicBlockQueue m_basic_block_queue;


   UInt64 m_instruction_count;
   UInt64 m_cycle_count;
};

#endif
