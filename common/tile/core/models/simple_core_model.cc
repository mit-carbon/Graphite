#include "core.h"
#include "log.h"
#include "simple_core_model.h"
#include "branch_predictor.h"
#include "clock_converter.h"
#include "tile.h"

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
   m_total_l1icache_stall_time = Time(0);
   m_total_l1dcache_read_stall_time = Time(0);
   m_total_l1dcache_write_stall_time = Time(0);
}

void SimpleCoreModel::outputSummary(std::ostream &os)
{
   CoreModel::outputSummary(os);
  
   //os << "    Total L1-I Cache Stall Time (in ns): " << m_total_l1icache_stall_time.toNanosec()<< endl;
   //os << "    Total L1-D Cache Read Stall Time (in ns): " << m_total_l1dcache_read_stall_time.toNanosec()<< endl;
   //os << "    Total L1-D Cache Write Stall Time (in ns): " << m_total_l1dcache_write_stall_time.toNanosec() << endl;
}

void SimpleCoreModel::handleInstruction(Instruction *instruction)
{
   // LOG_PRINT("Started processing instruction: Address(%#lx), Type(%u), Cost(%llu), Curr Time(%llu)",
   //       instruction->getAddress(), instruction->getType(), instruction->getCost(this).toNanosec(), m_curr_time.toNanosec());
   
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)
   Time cost = instruction->getCost(this);

   Time memory_stall_time(0);
   Time execution_unit_stall_time(0);

   // Instruction Memory Modeling
   Time instruction_memory_access_time = modelICache(instruction->getAddress(), instruction->getSize());
   memory_stall_time += instruction_memory_access_time;
   m_total_l1icache_stall_time += instruction_memory_access_time;

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

            Time access_time(info.memory_info.latency);
            memory_stall_time += access_time;
            m_total_l1dcache_read_stall_time += access_time;
            // ignore address
         }
         else
         {
            LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                             "Expected memory write info, got: %d.", info.type);

            Time access_time(info.memory_info.latency);
            memory_stall_time += access_time;
            m_total_l1dcache_write_stall_time += access_time;
            // ignore address
         }

         popDynamicInstructionInfo();
      }
   }

   if (instruction->isDynamic())
   {
      LOG_ASSERT_ERROR(memory_stall_time == 0, "memory_stall_time(%llu)", memory_stall_time.getTime());
      m_curr_time += cost;
   }
   else // Static Instruction
   {
      execution_unit_stall_time += cost;
      m_curr_time += (memory_stall_time + execution_unit_stall_time);
   }

   // update counters
   m_instruction_count++;

   // Update Common Counters
   updatePipelineStallCounters(instruction, memory_stall_time, execution_unit_stall_time);

   // LOG_PRINT("Finished processing instruction: Address(%#lx), Type(%u), Cost(%llu), Curr Time(%llu)",
   //       instruction->getAddress(), instruction->getType(), instruction->getCost(this).toNanosec(), m_curr_time.toNanosec());
}

Time SimpleCoreModel::modelICache(IntPtr ins_address, UInt32 ins_size)
{
   return m_core->readInstructionMemory(ins_address, ins_size);
}
