using namespace std;

#include "core.h"
#include "iocoom_performance_model.h"

#include "log.h"
#include "dynamic_instruction_info.h"
#include "config.hpp"
#include "simulator.h"
#include "branch_predictor.h"

IOCOOMPerformanceModel::IOCOOMPerformanceModel(Core *core, float frequency)
   : CoreModel(core, frequency)
   , m_instruction_count(0)
   , m_register_scoreboard(512)
   , m_store_buffer(0)
   , m_load_unit(0)
{
   config::Config *cfg = Sim()->getCfg();

   try
   {
      m_store_buffer = new StoreBuffer(cfg->getInt("perf_model/core/iocoom/num_store_buffer_entries",1));
      m_load_unit = new LoadUnit(cfg->getInt("perf_model/core/iocoom/num_outstanding_loads",3));
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Config info not available.");
   }

   initializeRegisterScoreboard();
}

IOCOOMPerformanceModel::~IOCOOMPerformanceModel()
{
   delete m_load_unit;
   delete m_store_buffer;
}

void IOCOOMPerformanceModel::outputSummary(std::ostream &os)
{
   os << "Core Performance Model Summary:" << endl;
   os << "    Instructions: " << m_instruction_count << std::endl;
   CoreModel::outputSummary(os);
}

void IOCOOMPerformanceModel::handleInstruction(Instruction *instruction)
{
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)

   // Model Instruction Cache Fetch
   UInt64 instruction_cache_access_latency = modelICache(instruction->getAddress(), instruction->getSize());
   m_cycle_count += (instruction_cache_access_latency - 1);

   UInt64 cost = instruction->getCost();
   
   /* 
      model instruction in the following steps:
      - find when read operations are available
      - find latency of instruction
      - update write operands
   */
   const OperandList &ops = instruction->getOperands();

   // buffer write operands to be updated after instruction executes
   DynamicInstructionInfoQueue write_info;

   // find when read operands are available
   UInt64 read_operands_ready = m_cycle_count;
   UInt64 max_load_latency = 0;

   // REG read operands
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if ( (o.m_direction != Operand::READ) || (o.m_type != Operand::REG) )
         continue;

      LOG_ASSERT_ERROR(o.m_value < m_register_scoreboard.size(),
                       "Register value out of range: %llu", o.m_value);

      if (read_operands_ready < m_register_scoreboard[o.m_value])
         read_operands_ready = m_register_scoreboard[o.m_value];
   }

   // MEMORY read & write operands
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if (o.m_type != Operand::MEMORY)
         continue;
         
      DynamicInstructionInfo &info = getDynamicInstructionInfo();

      if (o.m_direction == Operand::READ)
      {
         LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_READ,
                          "Expected memory read info, got: %d.", info.type);

         pair<UInt64,UInt64> load_timing_info = executeLoad(read_operands_ready, info);
         UInt64 load_ready = load_timing_info.first;
         UInt64 load_latency = load_timing_info.second;

         if (max_load_latency < load_latency)
            max_load_latency = load_latency;

         // This 'ready' is related to a structural hazard in the LOAD Unit
         if (read_operands_ready < load_ready)
            read_operands_ready = load_ready;
      }
      else
      {
         LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                          "Expected memory write info, got: %d.", info.type);

         write_info.push(info);
      }

      popDynamicInstructionInfo();
   }

   // Calculate the completion time of instruction (after fetching read operands + execution unit)
   UInt64 execute_unit_completion_time = read_operands_ready + max_load_latency + cost;

   // Time when write operands are ready
   UInt64 write_operands_ready = execute_unit_completion_time;

   // REG write operands
   // In this core model, we directly resolve WAR hazards since we wait
   // for all the read operands of an instruction to be available before
   // we issue it
   // Assume that the register file can be written in one cycle
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if ( (o.m_direction != Operand::WRITE) || (o.m_type != Operand::REG) )
         continue;

      // Note that m_cycle_count can be less then the previous value
      // of m_register_scoreboard[o.m_value]
      m_register_scoreboard[o.m_value] = execute_unit_completion_time;
      if (write_operands_ready < m_register_scoreboard[o.m_value])
         write_operands_ready = m_register_scoreboard[o.m_value];
   }

   // MEMORY write operands
   // This is done before doing register
   // operands to make sure the scoreboard is updated correctly
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if ( (o.m_direction != Operand::WRITE) || (o.m_type != Operand::MEMORY) )
         continue;

      const DynamicInstructionInfo &info = write_info.front();
      // This just updates the contents of the store buffer
      UInt64 store_time = executeStore(execute_unit_completion_time, info);
      write_info.pop();

      if (write_operands_ready < store_time)
         write_operands_ready = store_time;
   }

   // Completion Time of instruction
   UInt64 completion_time = write_operands_ready;

   // update cycle count with instruction cost
   // If it is a simple load instruction, execute the next instruction,
   // else wait till all the operands are fetched to execute the next instruction
   if (instruction->isSimpleMemoryLoad())
      m_cycle_count = read_operands_ready + cost;
   else
      m_cycle_count = completion_time;

   LOG_ASSERT_ERROR(write_info.empty(), "Some write info left over?");

   // instruction->print();
   
   // Update Statistics
   m_instruction_count++;
}

pair<UInt64,UInt64>
IOCOOMPerformanceModel::executeLoad(UInt64 time, const DynamicInstructionInfo &info)
{
   // similarly, a miss in the l1 with a completed entry in the store
   // buffer is treated as an invalidation
   StoreBuffer::Status status = m_store_buffer->isAddressAvailable(time, info.memory_info.addr);

   if (status == StoreBuffer::VALID)
      return make_pair<UInt64,UInt64>(time,0);

   // a miss in the l1 forces a miss in the store buffer
   UInt64 latency = info.memory_info.latency;

   return make_pair<UInt64,UInt64>(m_load_unit->execute(time, latency), latency);
}

UInt64 IOCOOMPerformanceModel::executeStore(UInt64 time, const DynamicInstructionInfo &info)
{
   UInt64 latency = info.memory_info.latency;

   return m_store_buffer->executeStore(time, latency, info.memory_info.addr);
}

UInt64 IOCOOMPerformanceModel::modelICache(IntPtr ins_address, UInt32 ins_size)
{
   return getCore()->readInstructionMemory(ins_address, ins_size);
}

void IOCOOMPerformanceModel::initializeRegisterScoreboard()
{
   for (unsigned int i = 0; i < m_register_scoreboard.size(); i++)
   {
      m_register_scoreboard[i] = 0;
   }
}

void IOCOOMPerformanceModel::reset()
{
   CoreModel::reset();

   m_instruction_count = 0;
   initializeRegisterScoreboard();
   m_store_buffer->reset();
   m_load_unit->reset();
}

// Helper classes 

IOCOOMPerformanceModel::LoadUnit::LoadUnit(unsigned int num_units)
   : m_scoreboard(num_units)
{
   initialize();
}

IOCOOMPerformanceModel::LoadUnit::~LoadUnit()
{
}

UInt64 IOCOOMPerformanceModel::LoadUnit::execute(UInt64 time, UInt64 occupancy)
{
   UInt64 unit = 0;

   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_scoreboard[i] <= time)
      {
         // a unit is available
         m_scoreboard[i] = time + occupancy;
         return time;
      }
      else
      {
         if (m_scoreboard[i] < m_scoreboard[unit])
            unit = i;
      }
   }

   // update unit, return time available
   m_scoreboard[unit] += occupancy;
   return m_scoreboard[unit] - occupancy;
}

void IOCOOMPerformanceModel::LoadUnit::initialize()
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
   }
}

void IOCOOMPerformanceModel::LoadUnit::reset()
{
   initialize();
}

IOCOOMPerformanceModel::StoreBuffer::StoreBuffer(unsigned int num_entries)
   : m_scoreboard(num_entries)
   , m_addresses(num_entries)
{
   initialize();
}

IOCOOMPerformanceModel::StoreBuffer::~StoreBuffer()
{
}

UInt64 IOCOOMPerformanceModel::StoreBuffer::executeStore(UInt64 time, UInt64 occupancy, IntPtr addr)
{
   // Note: basically identical to ExecutionUnit, except we need to
   // track addresses as well

   // is address already in buffer?
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_addresses[i] == addr)
      {
         m_scoreboard[i] = time + occupancy;
         return time;
      }
   }

   // if not, find earliest available entry
   unsigned int unit = 0;

   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_scoreboard[i] <= time)
      {
         // a unit is available
         m_scoreboard[i] = time + occupancy;
         m_addresses[i] = addr;
         return time;
      }
      else
      {
         if (m_scoreboard[i] < m_scoreboard[unit])
            unit = i;
      }
   }

   // update unit, return time available
   m_scoreboard[unit] += occupancy;
   m_addresses[unit] = addr;
   return m_scoreboard[unit] - occupancy;
}

IOCOOMPerformanceModel::StoreBuffer::Status IOCOOMPerformanceModel::StoreBuffer::isAddressAvailable(UInt64 time, IntPtr addr)
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      if (m_addresses[i] == addr)
      {
         if (m_scoreboard[i] >= time)
            return VALID;
      }
   }
   
   return NOT_FOUND;
}

void IOCOOMPerformanceModel::StoreBuffer::initialize()
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
      m_addresses[i] = 0;
   }
}

void IOCOOMPerformanceModel::StoreBuffer::reset()
{
   initialize();
}
