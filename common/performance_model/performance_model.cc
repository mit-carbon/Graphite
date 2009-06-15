#include "performance_model.h"
#include "simulator.h"

PerformanceModel::PerformanceModel()
{
}

PerformanceModel::~PerformanceModel()
{
}

// Public Interface
void PerformanceModel::queueDynamicInstruction(Instruction *i)
{
   if (!Config::getSingleton()->getEnablePerformanceModeling())
   {
      delete i;
      return;
   }

   BasicBlock *bb = new BasicBlock(true);
   bb->push_back(i);
   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(bb);
}

void PerformanceModel::queueBasicBlock(BasicBlock *basic_block)
{
   if (!Config::getSingleton()->getEnablePerformanceModeling())
      return;

//   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue_lock.acquire();
   m_basic_block_queue.push(basic_block);
}

//FIXME: this will go in a thread
void PerformanceModel::iterate()
{
//   ScopedLock sl(m_basic_block_queue_lock);

   while(m_basic_block_queue.size() > 1)
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
   m_basic_block_queue_lock.release();
}

void PerformanceModel::pushDynamicInstructionInfo(DynamicInstructionInfo &i)
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   m_dynamic_info_queue.push(i);
}

void PerformanceModel::popDynamicInstructionInfo()
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() > 0,
                    "Expected some dynamic info to be available.");
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");
   m_dynamic_info_queue.pop();
}

DynamicInstructionInfo& PerformanceModel::getDynamicInstructionInfo()
{
   ScopedLock sl(m_dynamic_info_queue_lock);
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() > 0,
                    "Expected some dynamic info to be available.");
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");
   return m_dynamic_info_queue.front();
}
