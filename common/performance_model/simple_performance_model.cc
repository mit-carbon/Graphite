#include "performance_model.h"
#include "log.h"

using std::endl;

SimplePerformanceModel::SimplePerformanceModel()
    : m_instruction_count(0)
    , m_cycle_count(0)
{
}

SimplePerformanceModel::~SimplePerformanceModel()
{
}

void SimplePerformanceModel::outputSummary(std::ostream &os)
{
   os << "  Instructions: " << getInstructionCount() << endl
      << "  Cycles: " << getCycleCount() << endl;
}

void SimplePerformanceModel::handleInstruction(Instruction *instruction)
{
   // compute cost
   UInt64 cost = 0;

   const OperandList &ops = instruction->getOperands();

   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_type == Operand::MEMORY)
      {
         if (o.m_direction == Operand::READ)
         {
            DynamicInstructionInfo &i = getDynamicInstructionInfo();
            LOG_ASSERT_ERROR(i.type == DynamicInstructionInfo::INFO_MEMORY,
                             "Expected memory info.");

            cost += i.memory_info.latency;
            // ignore address
         }
         
         popDynamicInstructionInfo();
      }
   }

   cost += instruction->getCost();

   // update counters
   m_instruction_count++;
   m_cycle_count += cost;
}

