#include "performance_model.h"
#include "simulator.h"

PerformanceModel::PerformanceModel()
{
}

PerformanceModel::~PerformanceModel()
{
}

// Public Interface
void PerformanceModel::queueInstruction(Instruction *i)
{
   ScopedLock sl(m_basic_block_queue_lock);
   BasicBlock *bb = new BasicBlock(true);
   bb->push_back(i);
   m_basic_block_queue.push(bb);
}

void PerformanceModel::queueBasicBlock(BasicBlock *basic_block)
{
   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(basic_block);
}

//FIXME: this will go in a thread
void PerformanceModel::iterate()
{
   ScopedLock sl(m_basic_block_queue_lock);
   while(!m_basic_block_queue.empty())
   {
      BasicBlock *current_bb = m_basic_block_queue.front();
      m_basic_block_queue.pop();

      for(BasicBlock::iterator i = current_bb->begin(); i != current_bb->end(); i++)
      {
          handleInstruction(*i);
      }

      if (current_bb->isDynamic())
         delete current_bb;
   }
}

void PerformanceModel::PushDynamicInstructionInfo(DynamicInstructionInfo &i)
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   m_dynamic_info_queue.push(i);
}

void PerformanceModel::PopDynamicInstructionInfo()
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   m_dynamic_info_queue.pop();
}

DynamicInstructionInfo& PerformanceModel::getDynamicInstructionInfo()
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   return m_dynamic_info_queue.front();
}
