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
   m_instruction_count++;
   m_cycle_count += instruction->getCost();
}

