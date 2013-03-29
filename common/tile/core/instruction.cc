#include "instruction.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "branch_predictor.h"

// Instruction

Instruction::StaticInstructionCosts Instruction::m_instruction_costs;

Instruction::Instruction(InstructionType type, UInt64 opcode, OperandList &operands)
   : m_type(type)
   , m_opcode(opcode)
   , m_address(0)
   , m_size(0)
   , m_operands(operands)
{
}

Instruction::Instruction(InstructionType type)
   : m_type(type)
   , m_opcode(0)
   , m_address(0)
   , m_size(0)
{
}

Time Instruction::getCost(CoreModel* perf)
{
   LOG_ASSERT_ERROR(m_type < MAX_INSTRUCTION_COUNT, "Unknown instruction type: %d", m_type);
   return perf->getCost(m_type); 
}

bool Instruction::isSimpleMemoryLoad() const
{
   if (m_operands.size() > 2)
      return false;

   bool memory_read = false;
   bool reg_write = false;
   for (unsigned int i = 0; i < m_operands.size(); i++)
   {
      const Operand& o = m_operands[i];
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::READ))
         memory_read = true;
      if ((o.m_type == Operand::REG) && (o.m_direction == Operand::WRITE))
         reg_write = true;
   }

   switch (m_operands.size())
   {
   case 1:
      return (memory_read);
   case 2:
      return (memory_read && reg_write);
   default:
      return false;
   }
}

void Instruction::initializeStaticInstructionModel()
{
   m_instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   for(unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       char key_name [1024];
       snprintf(key_name, 1024, "core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       UInt32 instruction_cost = 0;
       
       try
       {
          instruction_cost = Sim()->getCfg()->getInt(key_name);
       }
       catch (...)
       {
          LOG_PRINT_ERROR("Could not read instruction cost for (%s)", key_name);
       }

       m_instruction_costs[i] = instruction_cost;
   }
}

// BranchInstruction

BranchInstruction::BranchInstruction(UInt64 opcode, OperandList &l)
   : Instruction(INST_BRANCH, opcode, l)
{ }

Time BranchInstruction::getCost(CoreModel* perf)
{
   volatile float frequency = perf->getCore()->getTile()->getFrequency();
   BranchPredictor *bp = perf->getBranchPredictor();

   DynamicInstructionInfo &i = perf->getDynamicInstructionInfo();
   LOG_ASSERT_ERROR(i.type == DynamicInstructionInfo::BRANCH, "type(%u)", i.type);

   // branch prediction not modeled
   if (bp == NULL)
   {
      perf->popDynamicInstructionInfo();
      return Time(Latency(1,frequency));
   }

   bool prediction = bp->predict(getAddress(), i.branch_info.target);
   bool correct = (prediction == i.branch_info.taken);

   bp->update(prediction, i.branch_info.taken, getAddress(), i.branch_info.target);
   Latency cost = correct ? Latency(1,frequency) : Latency(bp->getMispredictPenalty(),frequency);
      
   perf->popDynamicInstructionInfo();
   return Time(cost);
}

// SpawnInstruction

SpawnInstruction::SpawnInstruction(Time cost)
   : DynamicInstruction(cost, INST_SPAWN)
{}

Time SpawnInstruction::getCost(CoreModel* perf)
{
   perf->setCurrTime(m_cost);
   throw CoreModel::AbortInstructionException(); // exit out of handleInstruction
}

// Instruction

void Instruction::print() const
{
   ostringstream out;
   out << "Address(0x" << hex << m_address << dec << ") Size(" << m_size << ") : ";
   for (unsigned int i = 0; i < m_operands.size(); i++)
   {
      const Operand& o = m_operands[i];
      o.print(out);
   }
   LOG_PRINT("%s", out.str().c_str());
}

// Operand

void Operand::print(ostringstream& out) const
{
   // Type
   if (m_type == REG)
   {
      out << "REG-";
      // Value
      out << m_value << "-";
      // Direction
      if (m_direction == READ)
         out << "READ, ";
      else
         out << "WRITE, ";
   }
   else if (m_type == MEMORY)
   {
      out << "MEMORY-";
      // Direction
      if (m_direction == READ)
         out << "READ, ";
      else
         out << "WRITE, ";
   }
   else if (m_type == IMMEDIATE)
   {
      out << "IMMEDIATE-";
      // Value
      out << m_value << ", ";
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Operand Type(%u)", m_type);
   }
}
