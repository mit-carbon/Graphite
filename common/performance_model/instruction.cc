#include "instruction.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "performance_model.h"

// Instruction

Instruction::StaticInstructionCosts Instruction::m_instruction_costs;

Instruction::Instruction(InstructionType type, OperandList &operands)
   : m_type(type)
   , m_operands(operands)
{
}

Instruction::Instruction(InstructionType type)
   : m_type(type)
{
}

InstructionType Instruction::getType()
{
    return m_type;
}

UInt64 Instruction::getCost()
{
   LOG_ASSERT_ERROR(m_type < MAX_INSTRUCTION_COUNT, "Unknown instruction type: %d", m_type);
   return Instruction::m_instruction_costs[m_type]; 
}

void Instruction::initializeStaticInstructionModel()
{
   m_instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   for(unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       char key_name [1024];
       snprintf(key_name, 1024, "perf_model/core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       UInt32 instruction_cost = Sim()->getCfg()->getInt(key_name, 0);
       m_instruction_costs[i] = instruction_cost;
   }
}

// DynamicInstruction

DynamicInstruction::DynamicInstruction(UInt64 cost, InstructionType type)
   : Instruction(type)
   , m_cost(cost)
{
}

DynamicInstruction::~DynamicInstruction()
{
}

UInt64 DynamicInstruction::getCost()
{
   return m_cost;
}

// StringInstruction

StringInstruction::StringInstruction(OperandList &ops)
   : Instruction(INST_STRING, ops)
{
}

UInt64 StringInstruction::getCost()
{
   // dequeue mem ops until we hit the final marker, then check count
   PerformanceModel *perf = Sim()->getCoreManager()->getCurrentCore()->getPerformanceModel();
   UInt32 count = 0;
   UInt64 cost = 0;
   DynamicInstructionInfo* i;

   while (true)
   {
      i = &perf->getDynamicInstructionInfo();

      if (i->type == DynamicInstructionInfo::STRING)
         break;

      LOG_ASSERT_ERROR(i->type == DynamicInstructionInfo::MEMORY_READ,
                       "Expected memory read in string instruction (or STRING).");

      cost += i->memory_info.latency;

      ++count;
      perf->popDynamicInstructionInfo();
   }

   LOG_ASSERT_ERROR(count == i->num_ops,
                    "Number of mem ops in queue doesn't match number in string instruction.");
   perf->popDynamicInstructionInfo();

   return cost;
}
