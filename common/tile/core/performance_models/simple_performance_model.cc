#include "core.h"
#include "log.h"
#include "simple_performance_model.h"
#include "branch_predictor.h"

using std::endl;

SimplePerformanceModel::SimplePerformanceModel(Core *core, float frequency)
    : CoreModel(core, frequency)
    , m_instruction_count(0)
{
}

SimplePerformanceModel::~SimplePerformanceModel()
{
}

void SimplePerformanceModel::outputSummary(std::ostream &os)
{
   os << "Core Performance Model Summary:" << endl;
   os << "    Instructions: " << getInstructionCount() << endl;
   CoreModel::outputSummary(os);
}

void SimplePerformanceModel::handleInstruction(Instruction *instruction)
{
   // Compute cost of the instruction
   UInt64 cost = 0;

   LOG_PRINT("Instruction Address(%#llx)", instruction->getAddress());

   if (instruction->getAddress() != 0)
   {
      // Not a dynamic instruction
      // Instruction Cache modeling
      DynamicInstructionInfo &icache_info = getDynamicInstructionInfo();
      LOG_ASSERT_ERROR(icache_info.type == DynamicInstructionInfo::MEMORY_READ, "Info Expected(%u), Got(%u)",
                       DynamicInstructionInfo::MEMORY_READ, icache_info.type);
      // Just add the time taken to access the icache
      cost += icache_info.memory_info.latency;
      popDynamicInstructionInfo();
   }

   const OperandList &ops = instruction->getOperands();
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_type == Operand::MEMORY)
      {
         DynamicInstructionInfo &info = getDynamicInstructionInfo();

         if (o.m_direction == Operand::READ)
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_READ,
                             "Expected memory read info, got: %d.", info.type);

            cost += info.memory_info.latency;
            // ignore address
         }
         else
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                             "Expected memory write info, got: %d.", info.type);

            cost += info.memory_info.latency;
            // ignore address
         }

         popDynamicInstructionInfo();
      }
   }

   cost += instruction->getCost();
   // LOG_ASSERT_WARNING(cost < 10000, "Cost is too big - cost:%llu, cycle_count: %llu, type: %d", cost, m_cycle_count, instruction->getType());

   // update counters
   m_instruction_count++;
   m_cycle_count += cost;
}

void SimplePerformanceModel::reset()
{
   CoreModel::reset();
   m_instruction_count = 0;
}
