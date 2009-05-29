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
    m_cycle_count++;
}
