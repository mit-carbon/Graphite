#include "performance_model.h"

PerformanceModel::PerformanceModel()
    : m_instruction_count(0)
    , m_cycle_count(0)
{
}

PerformanceModel::~PerformanceModel()
{
}

// Public Interface
void PerformanceModel::queueBasicBlock(BasicBlock *basic_block)
{
   m_basic_block_queue.push(basic_block);
}

//FIXME: this will go in a thread
void PerformanceModel::iterate()
{
   while(!m_basic_block_queue.empty())
   {
      BasicBlock *current_bb = m_basic_block_queue.front();
      m_basic_block_queue.pop();

      for(BasicBlock::iterator i = current_bb->begin(); i != current_bb->end(); i++)
          handleInstruction(*i);
   }
}

// Private Interface
void PerformanceModel::handleInstruction(Instruction *instruction)
{
   //FIXME: Put the instruction costs in the config file
   m_instruction_count++;
   switch(instruction->getInstructionType())
   {
       case INST_DIV:
           m_cycle_count += 25;
           break;
       case INST_MUL:
           m_cycle_count += 20;
           break;
       default:
           m_cycle_count++;
           break;
   }
}


