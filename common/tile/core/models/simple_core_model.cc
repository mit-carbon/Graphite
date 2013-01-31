#include "core.h"
#include "log.h"
#include "simple_core_model.h"
#include "branch_predictor.h"
#include "clock_converter.h"
#include "config.hpp"
#include "simulator.h"

using std::endl;

SimpleCoreModel::SimpleCoreModel(Core *core)
    : CoreModel(core)
{
   initializePipelineStallCounters();
}

SimpleCoreModel::~SimpleCoreModel()
{}

void SimpleCoreModel::initializePipelineStallCounters()
{
   m_total_l1icache_stall_cycles = 0;
   m_total_l1dcache_read_stall_cycles = 0;
   m_total_l1dcache_write_stall_cycles = 0;
}

void SimpleCoreModel::outputSummary(std::ostream &os)
{
   CoreModel::outputSummary(os);
  
//   float frequency = m_core->getTile()->getFrequency(); 
//   os << "    Total L1-I Cache Stall Time (in ns): " << convertCycleCount(m_total_l1icache_stall_cycles, frequency, 1.0) << endl;
//   os << "    Total L1-D Cache Read Stall Time (in ns): " << convertCycleCount(m_total_l1dcache_read_stall_cycles, frequency, 1.0) << endl;
//   os << "    Total L1-D Cache Write Stall Time (in ns): " << convertCycleCount(m_total_l1dcache_write_stall_cycles, frequency, 1.0) << endl;
}

void SimpleCoreModel::insertNOP()
{
   // Get NOP Cost
   UInt64 nop_cost = Sim()->getCfg()->getInt("core/static_instruction_costs/generic");
   // Update Cycle and Instruction Count
   m_cycle_count += nop_cost;
   m_instruction_count++;
}

void SimpleCoreModel::updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency)
{
   // Update Pipeline stall counters due to memory
   m_total_l1icache_stall_cycles = convertCycleCount(m_total_l1icache_stall_cycles, old_frequency, new_frequency);
   m_total_l1dcache_read_stall_cycles = convertCycleCount(m_total_l1dcache_read_stall_cycles, old_frequency, new_frequency);
   m_total_l1dcache_write_stall_cycles = convertCycleCount(m_total_l1dcache_write_stall_cycles, old_frequency, new_frequency);

   CoreModel::updateInternalVariablesOnFrequencyChange(old_frequency, new_frequency);
}

void SimpleCoreModel::handleInstruction(Instruction *instruction)
{
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)
   UInt64 cost = instruction->getCost();

   UInt64 memory_stall_cycles = 0;
   UInt64 execution_unit_stall_cycles = 0;

   // Instruction Memory Modeling
   UInt64 instruction_memory_access_latency = modelICache(instruction->getAddress(), instruction->getSize());
   memory_stall_cycles += instruction_memory_access_latency;
   m_total_l1icache_stall_cycles += instruction_memory_access_latency;

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

            memory_stall_cycles += info.memory_info.latency;
            m_total_l1dcache_read_stall_cycles += info.memory_info.latency;
            // ignore address
         }
         else
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                             "Expected memory write info, got: %d.", info.type);

            memory_stall_cycles += info.memory_info.latency;
            m_total_l1dcache_write_stall_cycles += info.memory_info.latency;
            // ignore address
         }

         popDynamicInstructionInfo();
      }
   }

   if (instruction->isDynamic())
   {
      LOG_ASSERT_ERROR(memory_stall_cycles == 0, "memory_stall_cycles(%llu)", memory_stall_cycles);
      m_cycle_count += cost;
   }
   else // Static Instruction
   {
      execution_unit_stall_cycles += cost;
      m_cycle_count += (memory_stall_cycles + execution_unit_stall_cycles);
   }

   // update counters
   m_instruction_count++;

   // Update Common Counters
   updatePipelineStallCounters(instruction, memory_stall_cycles, execution_unit_stall_cycles);
}

UInt64 SimpleCoreModel::modelICache(IntPtr ins_address, UInt32 ins_size)
{
   return m_core->readInstructionMemory(ins_address, ins_size);
}
