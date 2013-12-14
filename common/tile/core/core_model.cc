#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "simple_core_model.h"
#include "iocoom_core_model.h"
#include "branch_predictor.h"
#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "utils.h"
#include "time_types.h"
#include "mcpat_core_interface.h"
#include "remote_query_helper.h"

CoreModel* CoreModel::create(Core* core)
{
   string core_model = Config::getSingleton()->getCoreType(core->getTile()->getId());

   if (core_model == "iocoom")
      return new IOCOOMCoreModel(core);
   else if (core_model == "simple")
      return new SimpleCoreModel(core);
   else
   {
      LOG_PRINT_ERROR("Invalid core model type: %s", core_model.c_str());
      return NULL;
   }
}

// Public Interface
CoreModel::CoreModel(Core *core)
   : _core(core)
   , _curr_time(0)
   , _instruction_count(0)
   , _average_frequency(0.0)
   , _total_time(0)
   , _checkpointed_time(0)
   , _total_cycles(0)
   , _bp(0)
   , _instruction_queue(2)  // Max 2 instructions
   , _dynamic_memory_info_queue(3) // Max 3 dynamic memory info objects
   , _dynamic_branch_info_queue(1) // Max 1 dynamic branch info object
   , _enabled(false)
{
   // Create Branch Predictor
   _bp = BranchPredictor::create();

   // Initialize memory fence / dynamic instruction / pipeline stall Counters
   initializeMemoryFenceCounters();
   initializeDynamicInstructionCounters();
   initializePipelineStallCounters();

   // Initialize instruction costs
   initializeInstructionCosts(_core->getFrequency());
   
   LOG_PRINT("Initialized CoreModel.");
}

CoreModel::~CoreModel()
{
   delete _mcpat_core_interface;
   delete _bp; _bp = 0;
}


void CoreModel::initializeInstructionCosts(double frequency)
{
   _static_instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   _instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   for (unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       char key_name [1024];
       snprintf(key_name, 1024, "core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       _static_instruction_costs[i] = Sim()->getCfg()->getInt(key_name, 0);
       _instruction_costs[i] = Time(Latency(_static_instruction_costs[i], frequency));
   }
}

void CoreModel::updateInstructionCosts(double frequency)
{
   for (unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
      _instruction_costs[i] = Time(Latency(_static_instruction_costs[i], frequency));
}

const Time& CoreModel::getCost(InstructionType type) const
{
   return _instruction_costs[type];
}

void CoreModel::outputSummary(ostream& os, const Time& target_completion_time)
{
   os << "Core Summary:" << endl;
   os << "    Total Instructions: " << _instruction_count << endl;
   os << "    Completion Time (in nanoseconds): " << _curr_time.toNanosec() << endl;
   os << "    Average Frequency (in GHz): " << _average_frequency << endl;
   // Pipeline stall / dynamic instruction counters
   os << "    Synchronization Stalls: " << _total_sync_instructions << endl;
   os << "    Network Recv Stalls: " << _total_recv_instructions << endl;
   os << "    Stall Time Breakdown (in nanoseconds): " << endl;
   os << "      Memory: " << _total_memory_stall_time.toNanosec() << endl;
   os << "      Execution Unit: " << _total_execution_unit_stall_time.toNanosec() << endl;
   os << "      Synchronization: " << _total_sync_instruction_stall_time.toNanosec() << endl;
   os << "      Network Recv: " << _total_recv_instruction_stall_time.toNanosec() << endl;

   // Branch Predictor Summary
   if (_bp)
      _bp->outputSummary(os);

   _mcpat_core_interface->outputSummary(os, target_completion_time, _core->getFrequency());
   
   // Memory fence counters
   os << "    Fence Instructions: " << endl;
   os << "      Explicit LFENCE, SFENCE, MFENCE: " << _total_lfence_instructions + _total_sfence_instructions + _total_explicit_mfence_instructions << endl;
   os << "      Implicit MFENCE: " << _total_implicit_mfence_instructions << endl; 
}

void CoreModel::initializeMcPATInterface(UInt32 num_load_buffer_entries, UInt32 num_store_buffer_entries)
{
   // For Power/Area Modeling
   double frequency = _core->getFrequency();
   double voltage = _core->getVoltage();
   _mcpat_core_interface = new McPATCoreInterface(this, frequency, voltage, num_load_buffer_entries, num_store_buffer_entries);
}

void CoreModel::updateMcPATCounters(Instruction* instruction)
{
   // Get Branch Misprediction Count
   UInt64 total_branch_misprediction_count = _bp->getNumIncorrectPredictions();

   // Update Event Counters
   _mcpat_core_interface->updateEventCounters(instruction->getMcPATInstruction(),
                                              _curr_time.toCycles(_core->getFrequency()),
                                              total_branch_misprediction_count);
}

void CoreModel::computeEnergy(const Time& curr_time)
{
   _mcpat_core_interface->computeEnergy(curr_time, _core->getFrequency());
}

double CoreModel::getDynamicEnergy()
{
   return _mcpat_core_interface->getDynamicEnergy();
}

double CoreModel::getLeakageEnergy()
{
   return _mcpat_core_interface->getLeakageEnergy();
}

void CoreModel::enable()
{
   // Thread Spawner and MCP performance models should never be enabled
   if (_core->getTile()->getId() >= (tile_id_t) Config::getSingleton()->getApplicationTiles())
      return;

   _enabled = true;
}

void CoreModel::disable()
{
   _enabled = false;
}

// This function is called:
// 1) Whenever frequency is changed
void CoreModel::setDVFS(double old_frequency, double new_voltage, double new_frequency, const Time& curr_time)
{
   recomputeAverageFrequency(old_frequency);
   updateInstructionCosts(new_frequency);
   _mcpat_core_interface->setDVFS(old_frequency, new_voltage, new_frequency, curr_time);
}

void CoreModel::setCurrTime(Time time)
{
   _curr_time = time;
   _checkpointed_time = time;
}

// This function is called:
// 1) On thread exit
// 2) Whenever frequency is changed
void CoreModel::recomputeAverageFrequency(double old_frequency)
{
   _total_cycles += (_curr_time - _checkpointed_time).toCycles(old_frequency);
   _total_time += (_curr_time - _checkpointed_time);
   _average_frequency = ((double) _total_cycles)/((double) _total_time.toNanosec());

   _checkpointed_time = _curr_time;
}

void CoreModel::initializeMemoryFenceCounters()
{
   _total_lfence_instructions = 0;
   _total_sfence_instructions = 0;
   _total_explicit_mfence_instructions = 0;
   _total_implicit_mfence_instructions = 0;
}

void CoreModel::initializeDynamicInstructionCounters()
{
   _total_recv_instructions = 0;
   _total_sync_instructions = 0;
   _total_recv_instruction_stall_time = Time(0);
   _total_sync_instruction_stall_time = Time(0);
}

void CoreModel::initializePipelineStallCounters()
{
   _total_memory_stall_time = Time(0);
   _total_execution_unit_stall_time = Time(0);
}

Time CoreModel::modelICache(const Instruction* instruction)
{
   assert(!instruction->isDynamic());
   IntPtr address = instruction->getAddress();
   UInt32 size = instruction->getSize();
   return _core->readInstructionMemory(address, size);
}

void CoreModel::updateMemoryFenceCounters(const Instruction* instruction)
{
   if (instruction->isAtomic())
      _total_implicit_mfence_instructions ++;

   switch (instruction->getType())
   {
   case INST_LFENCE:
      _total_lfence_instructions ++;
      break;
   case INST_SFENCE:
      _total_sfence_instructions ++;
      break;
   case INST_MFENCE:
      _total_explicit_mfence_instructions ++;
      break;
   default:
      break;
   }
}

void CoreModel::updateDynamicInstructionCounters(const Instruction* instruction, const Time& cost)
{
   switch (instruction->getType())
   {
   case INST_RECV:
      _total_recv_instructions ++;
      _total_recv_instruction_stall_time += cost;
      break;
   case INST_SYNC:
      _total_sync_instructions ++;
      _total_sync_instruction_stall_time += cost;
      break;
   default:
      break;
   }
}

void CoreModel::updatePipelineStallCounters(const Time& memory_stall_time, const Time& execution_unit_stall_time)
{
   _total_memory_stall_time += memory_stall_time;
   _total_execution_unit_stall_time += execution_unit_stall_time;
}

void CoreModel::processDynamicInstruction(DynamicInstruction* i)
{
   if (_enabled)
   {
      try
      {
         handleInstruction(i);
      }
      catch (AbortInstructionException)
      {
         // move on to next ...
      }
   }
   delete i;
}

void CoreModel::queueInstruction(Instruction* instruction)
{
   if (!_enabled)
      return;
   assert(!_instruction_queue.full());
   _instruction_queue.push_back(instruction);
}

void CoreModel::iterate()
{
   while (_instruction_queue.size() > 1)
   {
      Instruction* instruction = _instruction_queue.front();
      handleInstruction(instruction);
      _instruction_queue.pop_front();
   }
}

void CoreModel::pushDynamicMemoryInfo(const DynamicMemoryInfo& info)
{
   if (!_enabled)
      return;
   assert(!_dynamic_memory_info_queue.full());
   _dynamic_memory_info_queue.push_back(info);
}

void CoreModel::popDynamicMemoryInfo()
{
   assert(_enabled);
   assert(!_dynamic_memory_info_queue.empty());
   _dynamic_memory_info_queue.pop_front();
}

const DynamicMemoryInfo& CoreModel::getDynamicMemoryInfo()
{
   return _dynamic_memory_info_queue.front();
}

void CoreModel::pushDynamicBranchInfo(const DynamicBranchInfo& info)
{
   if (!_enabled)
      return;
   assert(!_dynamic_branch_info_queue.full());
   _dynamic_branch_info_queue.push_back(info);
}

void CoreModel::popDynamicBranchInfo()
{
   assert(_enabled);
   assert(!_dynamic_branch_info_queue.empty());
   _dynamic_branch_info_queue.pop_front();
}

const DynamicBranchInfo& CoreModel::getDynamicBranchInfo()
{
   return _dynamic_branch_info_queue.front();
}
