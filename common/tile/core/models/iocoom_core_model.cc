using namespace std;

#include "core.h"
#include "iocoom_core_model.h"
#include "tile.h"
#include "dynamic_instruction_info.h"
#include "config.hpp"
#include "simulator.h"
#include "branch_predictor.h"
#include "clock_converter.h"
#include "tile.h"

IOCOOMCoreModel::IOCOOMCoreModel(Core *core)
   : CoreModel(core)
   , m_register_scoreboard(512)
   , m_register_wait_unit_list(512)
   , m_store_buffer(NULL)
   , m_load_buffer(NULL)
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
   double frequency = m_core->getFrequency();
   double voltage = m_core->getVoltage();
   m_mcpat_core_interface = new McPATCoreInterface(frequency, voltage, num_load_buffer_entries, num_store_buffer_entries);

   initializePipelineStallCounters();
}

IOCOOMCoreModel::~IOCOOMCoreModel()
{
   delete m_mcpat_core_interface;
   delete m_load_buffer;
   delete m_store_buffer;
}

void IOCOOMCoreModel::setDVFS(double old_frequency, double new_voltage, double new_frequency)
{
   CoreModel::setDVFS(old_frequency, new_voltage, new_frequency);
   m_mcpat_core_interface->setDVFS(new_voltage, new_frequency);
}

void IOCOOMCoreModel::initializePipelineStallCounters()
{
   m_total_load_buffer_stall_time = Time(0);
   m_total_store_buffer_stall_time = Time(0);
   m_total_l1icache_stall_time = Time(0);
   m_total_intra_ins_l1dcache_read_stall_time = Time(0);
   m_total_inter_ins_l1dcache_read_stall_time = Time(0);
   m_total_l1dcache_write_stall_time = Time(0);
   m_total_intra_ins_execution_unit_stall_time = Time(0);
   m_total_inter_ins_execution_unit_stall_time = Time(0);
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
      m_mcpat_core_interface->computeEnergy();
      m_mcpat_core_interface->displayEnergy(os);
   }

//   os << "    Total Load Buffer Stall Time (in ns): " << m_total_load_buffer_stall_time.toNanosec() << endl;
//   os << "    Total Store Buffer Stall Time (in ns): " << m_total_store_buffer_stall_time.toNanosec() << endl;
//   os << "    Total L1-I Cache Stall Time (in ns): " << m_total_l1icache_stall_time.toNanosec() << endl;
//   os << "    Total Intra Ins L1-D Cache Read Stall Time (in ns): " << m_total_intra_ins_l1dcache_read_stall_time.toNanosec() << endl;
//   os << "    Total Inter Ins L1-D Cache Read Stall Time (in ns): " << m_total_inter_ins_l1dcache_read_stall_time.toNanosec() << endl;
//   os << "    Total L1-D Cache Write Stall Time (in ns): " << m_total_l1dcache_write_stall_time.toNanosec() << endl;
//   os << "    Total Intra Ins Execution Unit Stall Time (in ns): " << m_total_intra_ins_execution_unit_stall_time.toNanosec() << endl;
//   os << "    Total Inter Ins Execution Unit Stall Time (in ns): " << m_total_inter_ins_execution_unit_stall_time.toNanosec() << endl;
//   os << "    Total Cycle Count: " << m_cycle_count << endl;
}

void IOCOOMCoreModel::computeEnergy()
{
   m_mcpat_core_interface->updateCycleCounters(m_curr_time.toCycles(m_core->getFrequency()));
   m_mcpat_core_interface->computeEnergy();
}

double IOCOOMCoreModel::getDynamicEnergy()
{
   double dynamic_energy = m_mcpat_core_interface->getDynamicEnergy();

   return dynamic_energy;
}
double IOCOOMCoreModel::getStaticPower()
{
   double static_power = m_mcpat_core_interface->getStaticPower();

   return static_power;
}

void IOCOOMCoreModel::handleInstruction(Instruction *instruction)
{
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)
   Time cost = instruction->getCost(this);

   Time one_cycle = Latency(1,m_core->getFrequency());

   // Model Instruction Fetch Stage
   Time instruction_ready = m_curr_time;
   Time instruction_memory_access_latency = modelICache(instruction->getAddress(), instruction->getSize());
   instruction_ready += (instruction_memory_access_latency - one_cycle);

   // Model instruction in the following steps:
   // - find when read operations are available
   // - find latency of instruction
   // - update write operands
   const OperandList &ops = instruction->getOperands();

   // buffer write operands to be updated after instruction executes
   DynamicInstructionInfoQueue write_info;

   // Time when register operands are ready (waiting for either the load unit or the execution unit)
   Time read_register_operands_ready_load_unit_wait = instruction_ready;
   Time read_register_operands_ready_execution_unit_wait = instruction_ready;

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
   Time read_register_operands_ready = max<Time>(read_register_operands_ready_load_unit_wait,
                                                     read_register_operands_ready_execution_unit_wait);
  
   // Assume memory is read only after all registers are read
   // This may be required since some registers may be used as the address for memory operations
   // Time when load unit and memory operands are ready
   Time load_buffer_ready = read_register_operands_ready;
   Time read_memory_operands_ready = read_register_operands_ready;
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

         pair<Time,Time> load_timing_info = executeLoad(read_register_operands_ready, info);
         Time load_ready = load_timing_info.first;
         Time load_latency = load_timing_info.second;
         Time load_completion_time = load_ready + load_latency;

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
   Time read_operands_ready = read_memory_operands_ready;

   // Calculate the completion time of instruction (after fetching read operands + execution unit)
   // Assume that there is no structural hazard at the execution unit
   Time execution_unit_completion_time = read_operands_ready + cost;

   // Time when write operands are ready
   Time write_operands_ready = execution_unit_completion_time;

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

   Time store_buffer_ready = write_operands_ready;
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
      Time store_time = executeStore(write_operands_ready, info);
      write_info.pop();

      if (store_buffer_ready < store_time)
         store_buffer_ready = store_time;
   }

   //                   ----->  time
   // -----------|-------------------------|---------------------------|----------------------------|-----------
   //    load_buffer_ready        read_operands_ready         write_operands_ready          store_buffer_ready
   //            |    load_latency         |            cost           |                            |
  
   Time memory_stall_time(0);
   Time execution_unit_stall_time(0);

   // update cycle count with instruction cost
   // If it is a simple load instruction, execute the next instruction after load_buffer_ready,
   // else wait till all the operands are fetched to execute the next instruction
   // Just add the cost for dynamic instructions since they involve pipeline stalls
   if (instruction->isDynamic())
   {
      m_curr_time += cost;
   }
   else // Static Instruction
   {
      // L1-I Cache
      memory_stall_time += (instruction_ready - m_curr_time);
      m_total_l1icache_stall_time += (instruction_ready - m_curr_time);

      // Register Read Operands
      execution_unit_stall_time += (read_register_operands_ready_execution_unit_wait - instruction_ready);
      m_total_inter_ins_execution_unit_stall_time += (read_register_operands_ready_execution_unit_wait - instruction_ready);
      memory_stall_time += (read_register_operands_ready - read_register_operands_ready_execution_unit_wait);
      m_total_inter_ins_l1dcache_read_stall_time += (read_register_operands_ready - read_register_operands_ready_execution_unit_wait);

      // Memory Read Operands - Load Buffer Stall
      memory_stall_time += (load_buffer_ready - read_register_operands_ready);
      m_total_load_buffer_stall_time += (load_buffer_ready - read_register_operands_ready);
      
      m_curr_time = load_buffer_ready + one_cycle;

      if (!instruction->isSimpleMemoryLoad())
      {
         // Memory Read Operands - Wait for L1-D Cache
         memory_stall_time += (read_memory_operands_ready - load_buffer_ready);
         m_total_intra_ins_l1dcache_read_stall_time += (read_memory_operands_ready - load_buffer_ready);

         m_curr_time = read_operands_ready + one_cycle;
         
         if (has_memory_write_operand)
         {
            // Wait till execution unit finishes
            execution_unit_stall_time += (write_operands_ready - read_operands_ready);
            m_total_intra_ins_execution_unit_stall_time += (write_operands_ready - read_operands_ready);
            // Memory Write Operands - Store Buffer Stall
            memory_stall_time += (store_buffer_ready - write_operands_ready);
            m_total_store_buffer_stall_time += (store_buffer_ready - write_operands_ready);

            m_curr_time = store_buffer_ready + one_cycle;

         }
      }
   }

   LOG_ASSERT_ERROR(write_info.empty(), "Some write info left over?");

   // Update Statistics
   m_instruction_count++;

   // Update Common Pipeline Stall Counters
   updatePipelineStallCounters(instruction, memory_stall_time, execution_unit_stall_time);

   // Get Branch Misprediction Count
   UInt64 m_total_branch_misprediction_count;
   m_total_branch_misprediction_count = getBranchPredictor()->getNumIncorrectPredictions();

   // Update Event Counters
   m_mcpat_core_interface->updateEventCounters(instruction, m_curr_time.toCycles(m_core->getFrequency()), m_total_branch_misprediction_count);
}

pair<Time,Time>
IOCOOMCoreModel::executeLoad(Time time, const DynamicInstructionInfo &info)
{
   // similarly, a miss in the l1 with a completed entry in the store
   // buffer is treated as an invalidation
   StoreBuffer::Status status = m_store_buffer->isAddressAvailable(time, info.memory_info.addr);

   if (status == StoreBuffer::VALID)
      return make_pair<Time,Time>(time,Time(0));

   // a miss in the l1 forces a miss in the store buffer
   Time latency(info.memory_info.latency);

   return make_pair<Time,Time>(m_load_buffer->execute(time, latency), latency);
}

Time IOCOOMCoreModel::executeStore(Time time, const DynamicInstructionInfo &info)
{
   Time latency = Time(info.memory_info.latency);

   return m_store_buffer->executeStore(time, latency, info.memory_info.addr);
}

Time IOCOOMCoreModel::modelICache(IntPtr ins_address, UInt32 ins_size)
{
   return m_core->readInstructionMemory(ins_address, ins_size);
}

void IOCOOMCoreModel::initializeRegisterScoreboard()
{
   for (unsigned int i = 0; i < m_register_scoreboard.size(); i++)
   {
      m_register_scoreboard[i] = Time(0);
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

Time IOCOOMCoreModel::LoadBuffer::execute(Time time, Time occupancy)
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
      m_scoreboard[i] = Time(0);
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

Time IOCOOMCoreModel::StoreBuffer::executeStore(Time time, Time occupancy, IntPtr addr)
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

IOCOOMCoreModel::StoreBuffer::Status IOCOOMCoreModel::StoreBuffer::isAddressAvailable(Time time, IntPtr addr)
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
      m_scoreboard[i] = Time(0);
      m_addresses[i] = 0;
   }
}
