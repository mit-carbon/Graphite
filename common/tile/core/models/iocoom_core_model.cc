using namespace std;

#include "core.h"
#include "iocoom_core_model.h"
#include "dynamic_instruction_info.h"
#include "config.hpp"
#include "simulator.h"
#include "branch_predictor.h"
#include "log.h"

IOCOOMCoreModel::IOCOOMCoreModel(Core *core, float frequency)
   : CoreModel(core, frequency)
   , m_register_scoreboard(512)
   , m_register_wait_unit_list(512)
   , m_store_buffer(0)
   , m_load_buffer(0)
{
   config::Config *cfg = Sim()->getCfg();

   UInt32 num_load_buffer_entries = 0;
   UInt32 num_store_buffer_entries = 0;
   try
   {
      num_load_buffer_entries = cfg->getInt("core/iocoom/num_outstanding_loads");
      num_store_buffer_entries = cfg->getInt("core/iocoom/num_store_buffer_entries");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Config info not available.");
   }

   m_load_buffer = new LoadBuffer(num_load_buffer_entries);
   m_store_buffer = new StoreBuffer(num_store_buffer_entries);
   
   initializeRegisterScoreboard();
   initializeRegisterWaitUnitList();
   
   m_enable_area_and_power_modeling = Config::getSingleton()->getEnableAreaModeling() || Config::getSingleton()->getEnablePowerModeling();

   // For Power and Area Modeling
   m_mcpat_core_interface = new McPATCoreInterface(cfg->getInt("general/technology_node", 45),
                                (UInt32) frequency * 1000, num_load_buffer_entries, num_store_buffer_entries);

   initializePipelineStallCounters();
}

IOCOOMCoreModel::~IOCOOMCoreModel()
{
   delete m_mcpat_core_interface;
   delete m_load_buffer;
   delete m_store_buffer;
}

void IOCOOMCoreModel::initializePipelineStallCounters()
{
   m_total_load_buffer_stall_cycles = 0;
   m_total_store_buffer_stall_cycles = 0;
   m_total_l1icache_stall_cycles = 0;
   m_total_intra_ins_l1dcache_read_stall_cycles = 0;
   m_total_inter_ins_l1dcache_read_stall_cycles = 0;
   m_total_l1dcache_write_stall_cycles = 0;
   m_total_intra_ins_execution_unit_stall_cycles = 0;
   m_total_inter_ins_execution_unit_stall_cycles = 0;
}

void IOCOOMCoreModel::outputSummary(std::ostream &os)
{
   CoreModel::outputSummary(os);
   
   m_mcpat_core_interface->displayStats(os);
   m_mcpat_core_interface->displayParam(os);
   if (m_enable_area_and_power_modeling)
   {
      os << "  Area and Power Model Summary:" << endl;
      // Compute Energy for Total Run
      m_mcpat_core_interface->computeMcPATCoreEnergy();
      m_mcpat_core_interface->displayMcPATCoreEnergy(os);
   }

//   os << "    Total Load Buffer Stall Time (in ns): " << (UInt64) ((double) m_total_load_buffer_stall_cycles / m_frequency) << endl;
//   os << "    Total Store Buffer Stall Time (in ns): " << (UInt64) ((double) m_total_store_buffer_stall_cycles / m_frequency) << endl;
//   os << "    Total L1-I Cache Stall Time (in ns): " << (UInt64) ((double) m_total_l1icache_stall_cycles / m_frequency) << endl;
//   os << "    Total Intra Ins L1-D Cache Read Stall Time (in ns): " << (UInt64) ((double) m_total_intra_ins_l1dcache_read_stall_cycles / m_frequency) << endl;
//   os << "    Total Inter Ins L1-D Cache Read Stall Time (in ns): " << (UInt64) ((double) m_total_inter_ins_l1dcache_read_stall_cycles / m_frequency) << endl;
//   os << "    Total L1-D Cache Write Stall Time (in ns): " << (UInt64) ((double) m_total_l1dcache_write_stall_cycles / m_frequency) << endl;
//   os << "    Total Intra Ins Execution Unit Stall Time (in ns): " << (UInt64) ((double) m_total_intra_ins_execution_unit_stall_cycles / m_frequency) << endl;
//   os << "    Total Inter Ins Execution Unit Stall Time (in ns): " << (UInt64) ((double) m_total_inter_ins_execution_unit_stall_cycles / m_frequency) << endl;
}

void IOCOOMCoreModel::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   volatile float old_frequency = m_frequency;
   volatile float new_frequency = frequency;

   // Update Pipeline stall counters due to memory
   m_total_load_buffer_stall_cycles = (UInt64) (((double) m_total_load_buffer_stall_cycles / old_frequency) * new_frequency);
   m_total_store_buffer_stall_cycles = (UInt64) (((double) m_total_store_buffer_stall_cycles / old_frequency) * new_frequency);
   m_total_l1icache_stall_cycles = (UInt64) (((double) m_total_l1icache_stall_cycles / old_frequency) * new_frequency);
   m_total_intra_ins_l1dcache_read_stall_cycles = (UInt64) (((double) m_total_intra_ins_l1dcache_read_stall_cycles / old_frequency) * new_frequency);
   m_total_inter_ins_l1dcache_read_stall_cycles = (UInt64) (((double) m_total_inter_ins_l1dcache_read_stall_cycles / old_frequency) * new_frequency);
   m_total_l1dcache_write_stall_cycles = (UInt64) (((double) m_total_l1dcache_write_stall_cycles / old_frequency) * new_frequency);
   m_total_intra_ins_execution_unit_stall_cycles = (UInt64) (((double) m_total_intra_ins_execution_unit_stall_cycles / old_frequency) * new_frequency);
   m_total_inter_ins_execution_unit_stall_cycles = (UInt64) (((double) m_total_inter_ins_execution_unit_stall_cycles / old_frequency) * new_frequency);

   CoreModel::updateInternalVariablesOnFrequencyChange(frequency);
}

void IOCOOMCoreModel::handleInstruction(Instruction *instruction)
{
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)
   UInt64 cost = instruction->getCost();

   // Model Instruction Fetch Stage
   UInt64 instruction_ready = m_cycle_count;
   UInt64 instruction_memory_access_latency = modelICache(instruction->getAddress(), instruction->getSize());
   instruction_ready += (instruction_memory_access_latency - 1);

   // Model instruction in the following steps:
   // - find when read operations are available
   // - find latency of instruction
   // - update write operands
   const OperandList &ops = instruction->getOperands();

   // buffer write operands to be updated after instruction executes
   DynamicInstructionInfoQueue write_info;

   // Time when register operands are ready (waiting for either the load unit or the execution unit)
   UInt64 read_register_operands_ready_load_unit_wait = instruction_ready;
   UInt64 read_register_operands_ready_execution_unit_wait = instruction_ready;

   // REG read operands
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      if ( (o.m_direction != Operand::READ) || (o.m_type != Operand::REG) )
         continue;

      LOG_ASSERT_ERROR(o.m_value < m_register_scoreboard.size(),
                       "Register value out of range: %llu", o.m_value);

      // Compute the ready time for registers that are waiting on the LOAD_UNIT
      // and on the EXECUTION_UNIT
      // The final ready time is the max of this
      if (m_register_wait_unit_list[o.m_value] == LOAD_UNIT)
      {
         if (read_register_operands_ready_load_unit_wait < m_register_scoreboard[o.m_value])
            read_register_operands_ready_load_unit_wait = m_register_scoreboard[o.m_value];
      }
      else if (m_register_wait_unit_list[o.m_value] == EXECUTION_UNIT)
      {
         if (read_register_operands_ready_execution_unit_wait < m_register_scoreboard[o.m_value])
            read_register_operands_ready_execution_unit_wait = m_register_scoreboard[o.m_value];
      }
      else
      {
         LOG_ASSERT_ERROR(m_register_scoreboard[o.m_value] <= instruction_ready,
                          "Unrecognized Core Unit(%u)", m_register_wait_unit_list[o.m_value]);
      }
   }
   
   // The read register ready time is the max of this
   UInt64 read_register_operands_ready = max<UInt64>(read_register_operands_ready_load_unit_wait,
                                                     read_register_operands_ready_execution_unit_wait);
  
   // Assume memory is read only after all registers are read
   // This may be required since some registers may be used as the address for memory operations
   // Time when load unit and memory operands are ready
   UInt64 load_buffer_ready = read_register_operands_ready;
   UInt64 read_memory_operands_ready = read_register_operands_ready;
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

         pair<UInt64,UInt64> load_timing_info = executeLoad(read_register_operands_ready, info);
         UInt64 load_ready = load_timing_info.first;
         UInt64 load_latency = load_timing_info.second;
         UInt64 load_completion_time = load_ready + load_latency;

         // This 'ready' is related to a structural hazard in the LOAD Unit
         if (load_buffer_ready < load_ready)
            load_buffer_ready = load_ready;
         
         // Read memory completion time is when all the read operands are available and ready for execution unit
         if (read_memory_operands_ready < load_completion_time)
            read_memory_operands_ready = load_completion_time;
      }
      else
      {
         LOG_ASSERT_ERROR(info.type == DynamicInstructionInfo::MEMORY_WRITE,
                          "Expected memory write info, got: %d.", info.type);

         write_info.push(info);
      }
      
      popDynamicInstructionInfo();
   }

   assert(read_memory_operands_ready >= load_buffer_ready);

   // Time when read operands (both register and memory) are ready
   UInt64 read_operands_ready = read_memory_operands_ready;

   // Calculate the completion time of instruction (after fetching read operands + execution unit)
   // Assume that there is no structural hazard at the execution unit
   UInt64 execution_unit_completion_time = read_operands_ready + cost;

   // Time when write operands are ready
   UInt64 write_operands_ready = execution_unit_completion_time;

   // REG write operands
   // In this core model, we directly resolve WAR hazards since we wait
   // for all the read operands of an instruction to be available before
   // we issue it
   // Assume that the register file can be written in one cycle
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      // REG write operands
      if ( (o.m_direction != Operand::WRITE) || (o.m_type != Operand::REG) )
         continue;

      // The only case where this assertion is not true is when the register is written
      // into but is never read before the next write operation. We assume
      // that this never happend
      // LOG_ASSERT_ERROR(write_operands_ready > m_register_scoreboard[o.m_value],
      //       "Write Operands Ready(%llu), Register Scoreboard Value(%llu)",
      //       write_operands_ready, m_register_scoreboard[o.m_value]);
      m_register_scoreboard[o.m_value] = write_operands_ready;

      // Update the unit that the register is waiting for
      if (instruction->isSimpleMemoryLoad())
         m_register_wait_unit_list[o.m_value] = LOAD_UNIT;
      else
         m_register_wait_unit_list[o.m_value] = EXECUTION_UNIT;
   }

   UInt64 store_buffer_ready = write_operands_ready;
   bool has_memory_write_operand = false;
   // MEMORY write operands
   // This is done before doing register
   // operands to make sure the scoreboard is updated correctly
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      // MEMORY write operands
      if ( (o.m_direction != Operand::WRITE) || (o.m_type != Operand::MEMORY) )
         continue;

      // Instruction has a MEMORY WRITE operand
      has_memory_write_operand = true;

      const DynamicInstructionInfo &info = write_info.front();
      // This just updates the contents of the store buffer
      UInt64 store_time = executeStore(write_operands_ready, info);
      write_info.pop();

      if (store_buffer_ready < store_time)
         store_buffer_ready = store_time;
   }

   //                   ----->  time
   // -----------|-------------------------|---------------------------|----------------------------|-----------
   //    load_buffer_ready        read_operands_ready         write_operands_ready          store_buffer_ready
   //            |    load_latency         |            cost           |                            |
  
   UInt64 memory_stall_cycles = 0;
   UInt64 execution_unit_stall_cycles = 0;

   // update cycle count with instruction cost
   // If it is a simple load instruction, execute the next instruction after load_buffer_ready,
   // else wait till all the operands are fetched to execute the next instruction
   // Just add the cost for dynamic instructions since they involve pipeline stalls
   if (instruction->isDynamic())
   {
      m_cycle_count += cost;
   }
   else // Static Instruction
   {
      // L1-I Cache
      memory_stall_cycles += (instruction_ready - m_cycle_count);
      m_total_l1icache_stall_cycles += (instruction_ready - m_cycle_count);

      // Register Read Operands
      execution_unit_stall_cycles += (read_register_operands_ready_execution_unit_wait - instruction_ready);
      m_total_inter_ins_execution_unit_stall_cycles += (read_register_operands_ready_execution_unit_wait - instruction_ready);
      memory_stall_cycles += (read_register_operands_ready - read_register_operands_ready_execution_unit_wait);
      m_total_inter_ins_l1dcache_read_stall_cycles += (read_register_operands_ready - read_register_operands_ready_execution_unit_wait);

      // Memory Read Operands - Load Buffer Stall
      memory_stall_cycles += (load_buffer_ready - read_register_operands_ready);
      m_total_load_buffer_stall_cycles += (load_buffer_ready - read_register_operands_ready);
      
      m_cycle_count = load_buffer_ready + 1;

      if (!instruction->isSimpleMemoryLoad())
      {
         // Memory Read Operands - Wait for L1-D Cache
         memory_stall_cycles += (read_memory_operands_ready - load_buffer_ready);
         m_total_intra_ins_l1dcache_read_stall_cycles += (read_memory_operands_ready - load_buffer_ready);

         m_cycle_count = read_operands_ready + 1;
         
         if (has_memory_write_operand)
         {
            // Wait till execution unit finishes
            execution_unit_stall_cycles += (write_operands_ready - read_operands_ready);
            m_total_intra_ins_execution_unit_stall_cycles += (write_operands_ready - read_operands_ready);
            // Memory Write Operands - Store Buffer Stall
            memory_stall_cycles += (store_buffer_ready - write_operands_ready);
            m_total_store_buffer_stall_cycles += (store_buffer_ready - write_operands_ready);

            m_cycle_count = store_buffer_ready + 1;
         }
      }
   }

   LOG_ASSERT_ERROR(write_info.empty(), "Some write info left over?");

   // Update Statistics
   m_instruction_count++;

   // Update Common Pipeline Stall Counters
   updatePipelineStallCounters(instruction, memory_stall_cycles, execution_unit_stall_cycles);

   // Update Event Counters
   m_mcpat_core_interface->updateEventCounters(instruction, m_cycle_count);
}

pair<UInt64,UInt64>
IOCOOMCoreModel::executeLoad(UInt64 time, const DynamicInstructionInfo &info)
{
   // similarly, a miss in the l1 with a completed entry in the store
   // buffer is treated as an invalidation
   StoreBuffer::Status status = m_store_buffer->isAddressAvailable(time, info.memory_info.addr);

   if (status == StoreBuffer::VALID)
      return make_pair<UInt64,UInt64>(time,0);

   // a miss in the l1 forces a miss in the store buffer
   UInt64 latency = info.memory_info.latency;

   return make_pair<UInt64,UInt64>(m_load_buffer->execute(time, latency), latency);
}

UInt64 IOCOOMCoreModel::executeStore(UInt64 time, const DynamicInstructionInfo &info)
{
   UInt64 latency = info.memory_info.latency;

   return m_store_buffer->executeStore(time, latency, info.memory_info.addr);
}

UInt64 IOCOOMCoreModel::modelICache(IntPtr ins_address, UInt32 ins_size)
{
   return getCore()->readInstructionMemory(ins_address, ins_size);
}

void IOCOOMCoreModel::initializeRegisterScoreboard()
{
   for (unsigned int i = 0; i < m_register_scoreboard.size(); i++)
   {
      m_register_scoreboard[i] = 0;
   }
}

void IOCOOMCoreModel::initializeRegisterWaitUnitList()
{
   for (unsigned int i = 0; i < m_register_wait_unit_list.size(); i++)
   {
      m_register_wait_unit_list[i] = INVALID_UNIT;
   }
}

// Helper classes 

IOCOOMCoreModel::LoadBuffer::LoadBuffer(unsigned int num_units)
   : m_scoreboard(num_units)
{
   initialize();
}

IOCOOMCoreModel::LoadBuffer::~LoadBuffer()
{
}

UInt64 IOCOOMCoreModel::LoadBuffer::execute(UInt64 time, UInt64 occupancy)
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

void IOCOOMCoreModel::LoadBuffer::initialize()
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
   }
}

IOCOOMCoreModel::StoreBuffer::StoreBuffer(unsigned int num_entries)
   : m_scoreboard(num_entries)
   , m_addresses(num_entries)
{
   initialize();
}

IOCOOMCoreModel::StoreBuffer::~StoreBuffer()
{
}

UInt64 IOCOOMCoreModel::StoreBuffer::executeStore(UInt64 time, UInt64 occupancy, IntPtr addr)
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

IOCOOMCoreModel::StoreBuffer::Status IOCOOMCoreModel::StoreBuffer::isAddressAvailable(UInt64 time, IntPtr addr)
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

void IOCOOMCoreModel::StoreBuffer::initialize()
{
   for (unsigned int i = 0; i < m_scoreboard.size(); i++)
   {
      m_scoreboard[i] = 0;
      m_addresses[i] = 0;
   }
}
