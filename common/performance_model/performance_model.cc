#include "performance_model.h"

PerformanceModel::PerformanceModel()
    : m_instruction_count(0)
    , m_cycle_count(0)
{
}

PerformanceModel::~PerformanceModel()
{
}

void PerformanceModel::handleInstruction(Instruction *instruction)
{
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
