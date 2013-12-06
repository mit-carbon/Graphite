#include "core.h"
#include "iocoom_core_model.h"
#include "dynamic_instruction_info.h"
#include "config.hpp"
#include "simulator.h"
#include "branch_predictor.h"
#include "tile.h"
#include "utils.h"
#include "log.h"

IOCOOMCoreModel::IOCOOMCoreModel(Core *core)
   : CoreModel(core)
{
   // Initialize load/store queues
   _load_queue = new LoadQueue(this);
   _store_queue = new StoreQueue(this);

   // Initialize register scoreboard
   _register_scoreboard.resize(_NUM_REGISTERS, Time(0));
   _register_dependency_list.resize(_NUM_REGISTERS, INVALID_UNIT); 
   
   // Initialize pipeline stall counters
   initializePipelineStallCounters();

   // For Power and Area Modeling
   UInt32 num_load_queue_entries = 0;
   UInt32 num_store_queue_entries = 0;
   try
   {
      num_load_queue_entries = Sim()->getCfg()->getInt("core/iocoom/num_load_queue_entries");
      num_store_queue_entries = Sim()->getCfg()->getInt("core/iocoom/num_store_queue_entries");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [core/iocoom] params from the config file");
   }

   // Initialize McPAT
   initializeMcPATInterface(num_load_queue_entries, num_store_queue_entries);
   
   // One Cycle   
   _ONE_CYCLE = Latency(1, _core->getFrequency());
}

IOCOOMCoreModel::~IOCOOMCoreModel()
{
   delete _store_queue;
   delete _load_queue;
}

void
IOCOOMCoreModel::initializePipelineStallCounters()
{
   _total_load_queue_stall_time = Time(0);
   _total_store_queue_stall_time = Time(0);
   _total_l1icache_stall_time = Time(0);
   _total_intra_ins_l1dcache_stall_time = Time(0);
   _total_inter_ins_l1dcache_stall_time = Time(0);
   _total_intra_ins_execution_unit_stall_time = Time(0);
   _total_inter_ins_execution_unit_stall_time = Time(0);
}

void
IOCOOMCoreModel::outputSummary(std::ostream &os, const Time& target_completion_time)
{
   CoreModel::outputSummary(os, target_completion_time);

   os << "    Detailed Stall Time Breakdown (in nanoseconds): " << endl;
   os << "      Load Queue: "  << _total_load_queue_stall_time.toNanosec() << endl;
   os << "      Store Queue: " << _total_store_queue_stall_time.toNanosec() << endl;
   os << "      L1-I Cache: "  << _total_l1icache_stall_time.toNanosec() << endl;
   os << "      L1-D Cache (Intra-Instruction): " << _total_intra_ins_l1dcache_stall_time.toNanosec() << endl;
   os << "      L1-D Cache (Inter-Instruction): " << _total_inter_ins_l1dcache_stall_time.toNanosec() << endl;
   os << "      Execution Unit (Intra-Instruction): " << _total_intra_ins_execution_unit_stall_time.toNanosec() << endl;
   os << "      Execution Unit (Inter-Instruction): " << _total_inter_ins_execution_unit_stall_time.toNanosec() << endl;
}

void IOCOOMCoreModel::handleInstruction(Instruction *instruction)
{
   // Execute this first so that instructions have the opportunity to
   // abort further processing (via AbortInstructionException)
   Time cost = instruction->getCost(this);

   // Update Statistics
   _instruction_count++;

   if (instruction->isDynamic())
   {
      _curr_time += cost;
      updateDynamicInstructionCounters(instruction, cost);
      return;
   }

   // Model Instruction Fetch Stage
   Time instruction_ready = _curr_time;
 
   Time instruction_memory_access_latency = modelICache(instruction);
   if (instruction_memory_access_latency >= _ONE_CYCLE)
      instruction_memory_access_latency -= _ONE_CYCLE;
   instruction_ready += instruction_memory_access_latency;

   // Model instruction in the following steps:
   // - find when read operations are available
   // - find latency of instruction
   // - update write operands
   const RegisterOperandList& read_register_operands = instruction->getReadRegisterOperands();
   const RegisterOperandList& write_register_operands = instruction->getWriteRegisterOperands();
   UInt32 num_read_memory_operands = instruction->getNumReadMemoryOperands();
   UInt32 num_write_memory_operands = instruction->getNumWriteMemoryOperands();

   // Time when register operands are ready (waiting for either the load unit or the execution unit)
   Time register_operands_ready__load_unit = instruction_ready;
   Time register_operands_ready__execution_unit = instruction_ready;

   // REGISTER read operands
   for (unsigned int i = 0; i < read_register_operands.size(); i++)
   {
      const RegisterOperand& reg = read_register_operands[i];
      LOG_ASSERT_ERROR(reg < _register_scoreboard.size(), "Register value out of range: %u", reg);

      // Compute the ready time for registers that are waiting on the LOAD_UNIT
      // and on the EXECUTION_UNIT
      // The final ready time is the max of this
      if (_register_dependency_list[reg] == LOAD_UNIT)
      {
         if (register_operands_ready__load_unit < _register_scoreboard[reg])
            register_operands_ready__load_unit = _register_scoreboard[reg];
      }
      else if (_register_dependency_list[reg] == EXECUTION_UNIT)
      {
         if (register_operands_ready__execution_unit < _register_scoreboard[reg])
            register_operands_ready__execution_unit = _register_scoreboard[reg];
      }
      else
      {
         LOG_ASSERT_ERROR(_register_scoreboard[reg] <= instruction_ready,
                          "Unrecognized Unit(%u)", _register_dependency_list[reg]);
      }
   }
   
   // The read register ready time is the max of this
   Time register_operands_ready = getMax<Time>(register_operands_ready__load_unit,
                                               register_operands_ready__execution_unit);
  
   // Assume memory is read only after all registers are read
   // This may be required since some registers may be used as the address for memory operations
   // Time when load unit and memory operands are ready
   Time load_queue_ready = register_operands_ready;
   Time read_memory_operands_ready = register_operands_ready;
   // MEMORY read & write operands
   for (unsigned int i = 0; i < num_read_memory_operands; i++)
   {
      const DynamicMemoryInfo &info = getDynamicMemoryInfo();
      LOG_ASSERT_ERROR(info._read, "Expected memory read info");

      pair<Time,Time> load_timing_info = executeLoad(register_operands_ready, info);
      Time load_allocate_time = load_timing_info.first;
      Time load_completion_time = load_timing_info.second;

      // This 'ready' is related to a structural hazard in the LOAD Unit
      if (load_queue_ready < load_allocate_time)
         load_queue_ready = load_allocate_time;
      
      // Read memory completion time is when all the read operands are available and ready for execution unit
      if (read_memory_operands_ready < load_completion_time)
         read_memory_operands_ready = load_completion_time;
      
      popDynamicMemoryInfo();
   }

   assert(read_memory_operands_ready >= load_queue_ready);

   // Time when read operands (both register and memory) are ready
   Time read_operands_ready = read_memory_operands_ready;

   // Calculate the completion time of instruction (after fetching read operands + execution unit)
   // Assume that there is no structural hazard at the execution unit
   Time execution_unit_completion_time = read_operands_ready + cost;

   // Time when write operands are ready
   Time write_operands_ready = execution_unit_completion_time;

   // REGISTER write operands
   // In this core model, we directly resolve WAR hazards since we wait
   // for all the read operands of an instruction to be available before
   // we issue it
   // Assume that the register file can be written in one cycle
   for (unsigned int i = 0; i < write_register_operands.size(); i++)
   {
      const RegisterOperand& reg = write_register_operands[i];

      // The only case where this assertion is not true is when the register is written
      // into but is never read before the next write operation. We assume
      // that this never happened
      _register_scoreboard[reg] = write_operands_ready;
      // Update the unit that the register is waiting for
      _register_dependency_list[reg] = (instruction->isSimpleMovMemoryLoad()) ?
                                       LOAD_UNIT : EXECUTION_UNIT;
   }

   Time store_queue_ready = write_operands_ready;
   // MEMORY write operands
   // This is done before doing register
   // operands to make sure the scoreboard is updated correctly
   for (unsigned int i = 0; i < num_write_memory_operands; i++)
   {
      const DynamicMemoryInfo& info = getDynamicMemoryInfo();
      LOG_ASSERT_ERROR(!info._read, "Expected memory write info");
      
      // This just updates the contents of the store buffer
      Time store_allocate_time = executeStore(write_operands_ready, info);

      if (store_queue_ready < store_allocate_time)
         store_queue_ready = store_allocate_time;
 
      popDynamicMemoryInfo();
   }

   //                   ----->  time
   // -----------|-------------------------|---------------------------|----------------------------|-----------
   //    load_queue_ready        read_operands_ready         write_operands_ready          store_queue_ready
   //            |    load_latency         |            cost           |                            |
  
   Time memory_stall_time(0);
   Time execution_unit_stall_time(0);

   // update cycle count with instruction cost
   // If it is a simple load instruction, execute the next instruction after load_queue_ready,
   // else wait till all the operands are fetched to execute the next instruction
   // Just add the cost for dynamic instructions since they involve pipeline stalls

   // L1-I Cache
   memory_stall_time += (instruction_ready - _curr_time);
   _total_l1icache_stall_time += (instruction_ready - _curr_time);

   // Register Read Operands
   execution_unit_stall_time += (register_operands_ready__execution_unit - instruction_ready);
   _total_inter_ins_execution_unit_stall_time += (register_operands_ready__execution_unit - instruction_ready);
   memory_stall_time += (register_operands_ready - register_operands_ready__execution_unit);
   _total_inter_ins_l1dcache_stall_time += (register_operands_ready - register_operands_ready__execution_unit);

   // Memory Read Operands - Load Buffer Stall
   memory_stall_time += (load_queue_ready - register_operands_ready);
   _total_load_queue_stall_time += (load_queue_ready - register_operands_ready);
   
   _curr_time = load_queue_ready;

   if (!instruction->isSimpleMovMemoryLoad())
   {
      // Memory Read Operands - Wait for L1-D Cache
      memory_stall_time += (read_memory_operands_ready - load_queue_ready);
      _total_intra_ins_l1dcache_stall_time += (read_memory_operands_ready - load_queue_ready);

      _curr_time = read_operands_ready;
      
      if (num_write_memory_operands > 0)
      {
         // Wait till execution unit finishes
         execution_unit_stall_time += (write_operands_ready - read_operands_ready);
         _total_intra_ins_execution_unit_stall_time += (write_operands_ready - read_operands_ready);
         // Memory Write Operands - Store Buffer Stall
         memory_stall_time += (store_queue_ready - write_operands_ready);
         _total_store_queue_stall_time += (store_queue_ready - write_operands_ready);

         _curr_time = store_queue_ready;

      }
   }

   // Update memory fence / pipeline stall counters
   updateMemoryFenceCounters(instruction);
   updatePipelineStallCounters(memory_stall_time, execution_unit_stall_time);

   // Update McPAT counters
   updateMcPATCounters(instruction);
}

pair<Time,Time>
IOCOOMCoreModel::executeLoad(const Time& schedule_time, const DynamicMemoryInfo& info)
{
   Time load_latency(info._latency);
   // Increment by time taken to check the store queue
   load_latency += _ONE_CYCLE;
   
   // Check if data is present in store queue, If yes, just bypass
   StoreQueue::Status status = _store_queue->isAddressAvailable(schedule_time, info._address);
   if (status == StoreQueue::VALID)
      return make_pair(schedule_time, schedule_time + _ONE_CYCLE);
   else // (status != StoreQueue::VALID)
      return _load_queue->execute(schedule_time, load_latency);
}

Time
IOCOOMCoreModel::executeStore(const Time& schedule_time, const DynamicMemoryInfo& info)
{
   Time store_latency(info._latency);
   // Increment by time taken to check the load queue
   store_latency += _ONE_CYCLE;
  
   // Find the last load deallocate time
   Time last_load_deallocate_time = _load_queue->getLastDeallocateTime();
   return _store_queue->execute(schedule_time, store_latency, last_load_deallocate_time, info._address);
}

// Load Queue 

IOCOOMCoreModel::LoadQueue::LoadQueue(CoreModel* core_model)
{
   try
   {
      _num_entries = Sim()->getCfg()->getInt("core/iocoom/num_load_queue_entries");
      _speculative_loads_enabled = Sim()->getCfg()->getBool("core/iocoom/speculative_loads_enabled");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [core/iocoom] parameters from the cfg file");
   }
   _scoreboard.resize(_num_entries, Time(0));
   _allocate_idx = 0;
   // One Cycle   
   _ONE_CYCLE = Latency(1, core_model->getCore()->getFrequency());
}

IOCOOMCoreModel::LoadQueue::~LoadQueue()
{
}

pair<Time,Time>
IOCOOMCoreModel::LoadQueue::execute(const Time& schedule_time, const Time& load_latency)
{
   // Issue loads to cache hierarchy one by one
   Time allocate_time = getMax<Time>(_scoreboard[_allocate_idx], schedule_time);
   Time completion_time;
   Time deallocate_time;
   UInt32 last_idx = (_allocate_idx + _num_entries-1) % (_num_entries);
   if (_speculative_loads_enabled)
   {
      // With speculative loads, issue_time = allocate_time
      Time issue_time = allocate_time;
      completion_time = issue_time + load_latency;
      // The load queue should be de-allocated in order for memory consistency purposes
      // Assumption: Only one load can be deallocated per cycle
      deallocate_time = getMax<Time>(completion_time, _scoreboard[last_idx] + _ONE_CYCLE);
   }
   else // (!_speculative_loads_enabled)
   {
      // With non-speculative loads, loads can be issued and completed only in FIFO order
      Time issue_time = getMax<Time>(_scoreboard[last_idx], schedule_time);
      completion_time = issue_time + load_latency;
      deallocate_time = completion_time;
   }
   _scoreboard[_allocate_idx] = deallocate_time;
   _allocate_idx = (_allocate_idx + 1) % (_num_entries);
   return make_pair(allocate_time, completion_time);
}

const Time&
IOCOOMCoreModel::LoadQueue::getLastDeallocateTime()
{
   UInt32 last_idx = (_allocate_idx + _num_entries-1) % _num_entries;
   return _scoreboard[last_idx];
}

ostringstream&
operator<<(ostringstream& os, const IOCOOMCoreModel::LoadQueue& queue)
{
   os << "LoadQueue: (";
   for (UInt32 i = 0; i < queue._num_entries; i++)
   {
      const Time& deallocate_time = queue._scoreboard[i];
      os << deallocate_time.toNanosec() << ", ";
   }
   os << ")" << endl;
   return os;
}

// Store Queue

IOCOOMCoreModel::StoreQueue::StoreQueue(CoreModel* core_model)
{
   // The assumption is the store queue is reused as a store buffer
   // Committed stores have an additional "C" flag enabled
   try
   {
      _num_entries = Sim()->getCfg()->getInt("core/iocoom/num_store_queue_entries");
      _multiple_outstanding_RFOs_enabled = Sim()->getCfg()->getBool("core/iocoom/multiple_outstanding_RFOs_enabled");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read [core/iocoom] params from the cfg file");
   }

   _scoreboard.resize(_num_entries, Time(0));
   _addresses.resize(_num_entries, INVALID_ADDRESS);
   _allocate_idx = 0;
   // One Cycle   
   _ONE_CYCLE = Latency(1, core_model->getCore()->getFrequency());
}

IOCOOMCoreModel::StoreQueue::~StoreQueue()
{
}

Time
IOCOOMCoreModel::StoreQueue::execute(const Time& schedule_time, const Time& store_latency,
                                     const Time& last_load_deallocate_time, IntPtr address)
{
   // Note: basically identical to LoadQueue, except we need to track addresses as well.
   // We can't do store buffer coalescing. It violates x86 TSO memory consistency model.
   
   Time allocate_time = getMax<Time>(_scoreboard[_allocate_idx], schedule_time);
   Time deallocate_time;
   UInt32 last_idx = (_allocate_idx + _num_entries-1) % (_num_entries);
   const Time& last_store_deallocate_time = _scoreboard[last_idx];
   
   if (_multiple_outstanding_RFOs_enabled)
   {
      // With multiple outstanding RFOs, issue_time = allocate_time
      Time issue_time = allocate_time;
      Time completion_time = issue_time + store_latency;
      // The store queue should be de-allocated in order for memory consistency purposes
      // Assumption: Only one store can be deallocated per cycle
      deallocate_time = getMax<Time>(completion_time, last_store_deallocate_time + _ONE_CYCLE, last_load_deallocate_time);
   }
   else // (!_multiple_outstanding_RFOs_enabled)
   {
      // With multiple outstanding RFOs disabled, stores can be issued and completed only in FIFO order
      Time issue_time = getMax<Time>(schedule_time, last_store_deallocate_time, last_load_deallocate_time);
      Time completion_time = issue_time + store_latency;
      deallocate_time = completion_time;
   }
   _scoreboard[_allocate_idx] = deallocate_time;
   _addresses[_allocate_idx] = address;
   _allocate_idx = (_allocate_idx + 1) % (_num_entries);
   return allocate_time;
}

const Time&
IOCOOMCoreModel::StoreQueue::getLastDeallocateTime()
{
   UInt32 last_idx = (_allocate_idx + _num_entries-1) % _num_entries;
   return _scoreboard[last_idx];
}

IOCOOMCoreModel::StoreQueue::Status
IOCOOMCoreModel::StoreQueue::isAddressAvailable(const Time& schedule_time, IntPtr address)
{
   for (unsigned int i = 0; i < _scoreboard.size(); i++)
   {
      if (_addresses[i] == address)
      {
         if (_scoreboard[i] >= schedule_time)
            return VALID;
      }
   }
   return NOT_FOUND;
}

ostringstream&
operator<<(ostringstream& os, const IOCOOMCoreModel::StoreQueue& queue)
{
   os << "StoreQueue (";
   for (UInt32 i = 0; i < queue._num_entries; i++)
   {
      const Time& deallocate_time = queue._scoreboard[i];
      const IntPtr& address = queue._addresses[i];
      os << "<" << deallocate_time.toNanosec() << "," << hex << address << dec << ">, ";
   }
   os << ")" << endl;
   return os;
}

