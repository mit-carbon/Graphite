#include "instruction.h"
#include "simulator.h"

Instruction::StaticInstructionCosts Instruction::m_instruction_costs;

InstructionType Instruction::getInstructionType()
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
       snprintf(key_name, 1024, "core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       UInt32 instruction_cost = Sim()->getCfg()->getInt(key_name, 0);
       m_instruction_costs[i] = instruction_cost;
   }
}

