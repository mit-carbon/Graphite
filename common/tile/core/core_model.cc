#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "simple_core_model.h"
#include "iocoom_core_model.h"
#include "branch_predictor.h"
#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "fxsupport.h"
#include "clock_converter.h"
#include "utils.h"
#include "time_types.h"

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
   : m_core(core)
   , m_curr_time(0)
   , m_instruction_count(0)
   , m_average_frequency(0.0)
   , m_total_time(0)
   , m_checkpointed_time(0)
   , m_total_cycles(0)
   , m_enabled(false)
   , m_bp(0)
{
   // Create Branch Predictor
   m_bp = BranchPredictor::create();

   // Initialize Pipeline Stall Counters
   initializePipelineStallCounters();

   // Initialize instruction costs
   initializeCoreStaticInstructionModel(core->getTile()->getFrequency());
   LOG_PRINT("Initialized CoreModel.");
}

CoreModel::~CoreModel()
{
   delete m_bp; m_bp = 0;
}


void CoreModel::initializeCoreStaticInstructionModel(volatile float frequency)
{
   m_core_instruction_costs.resize(MAX_INSTRUCTION_COUNT);
   for(unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       char key_name [1024];
       snprintf(key_name, 1024, "core/static_instruction_costs/%s", INSTRUCTION_NAMES[i]);
       UInt32 instruction_cost = Sim()->getCfg()->getInt(key_name, 0);
       m_core_instruction_costs[i] = Time(Latency(instruction_cost,frequency));
   }
}

void CoreModel::updateCoreStaticInstructionModel(volatile float frequency)
{
   Instruction::StaticInstructionCosts instruction_costs = Instruction::getStaticInstructionCosts();
   for(unsigned int i = 0; i < MAX_INSTRUCTION_COUNT; i++)
   {
       m_core_instruction_costs[i] = Time(Latency(instruction_costs[i],frequency));
   }
}

Time CoreModel::getCost(InstructionType type)
{
   return m_core_instruction_costs[type];
}

void CoreModel::outputSummary(ostream& os)
{
   os << "Core Summary:" << endl;
   os << "    Total Instructions: " << m_instruction_count << endl;
   os << "    Completion Time (in ns): " << m_curr_time.toNanosec() << endl;
   os << "    Average Frequency (in GHz): " << m_average_frequency << endl;
   
   os << "    Total Synchronization Stalls: " << m_total_sync_instructions << endl;
   os << "    Total Network Recv Stalls: " << m_total_recv_instructions << endl;
   os << "    Total Memory Stall Time (in ns): " << m_total_memory_stall_time.toNanosec() << endl;
   os << "    Total Execution Unit Stall Time (in ns): " << m_total_execution_unit_stall_time.toNanosec() << endl;
   os << "    Total Synchronization Stall Time (in ns): " << m_total_sync_instruction_stall_time.toNanosec() << endl;
   os << "    Total Network Recv Stall Time (in ns): " << m_total_recv_instruction_stall_time.toNanosec() << endl;

   // Branch Predictor Summary
   if (m_bp)
      m_bp->outputSummary(os);
}

void CoreModel::enable()
{
   // Thread Spawner and MCP performance models should never be enabled
   if (m_core->getTile()->getId() >= (tile_id_t) Config::getSingleton()->getApplicationTiles())
      return;

   m_enabled = true;
}

void CoreModel::disable()
{
   m_enabled = false;
}

// This function is called:
// 1) Whenever frequency is changed
void CoreModel::updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency)
{
   recomputeAverageFrequency(old_frequency);
   updateCoreStaticInstructionModel(new_frequency);
}

void CoreModel::setCurrTime(Time time)
{
   m_curr_time = time;
   m_checkpointed_time = time;
}

// This function is called:
// 1) On thread exit
// 2) Whenever frequency is changed
void CoreModel::recomputeAverageFrequency(float old_frequency)
{
   m_total_cycles += (m_curr_time - m_checkpointed_time).toCycles(old_frequency);
   m_total_time += (m_curr_time - m_checkpointed_time);
   m_average_frequency = ((double) m_total_cycles)/((double) m_total_time.toNanosec());

   m_checkpointed_time = m_curr_time;
}

void CoreModel::initializePipelineStallCounters()
{
   m_total_recv_instructions = 0;
   m_total_sync_instructions = 0;
   m_total_recv_instruction_stall_time = Time(0);
   m_total_sync_instruction_stall_time = Time(0);
   m_total_memory_stall_time = Time(0);
   m_total_execution_unit_stall_time = Time(0);
}

void CoreModel::updatePipelineStallCounters(Instruction* i, Time memory_stall_time, Time execution_unit_stall_time)
{
   switch (i->getType())
   {
   case INST_RECV:
      m_total_recv_instructions ++;
      m_total_recv_instruction_stall_time += i->getCost(this);
      break;

   case INST_SYNC:
      m_total_sync_instructions ++;
      m_total_sync_instruction_stall_time += i->getCost(this);
      break;

   default:
      break;
   }
   
   m_total_memory_stall_time += memory_stall_time;
   m_total_execution_unit_stall_time += execution_unit_stall_time;
}

void CoreModel::processDynamicInstruction(DynamicInstruction* i)
{
   if (m_enabled)
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
   if (!m_enabled)
      return;
   m_instruction_queue.push(instruction);
}

void CoreModel::iterate()
{
   // Because we will sometimes not have info available (we will throw
   // a DynamicInstructionInfoNotAvailable)

   while (m_instruction_queue.size() > 1)
   {
      LOG_PRINT("Instruction Queue Size(%lu)", m_instruction_queue.size());
      Instruction* instruction = m_instruction_queue.front();

      try
      {
         handleInstruction(instruction);
         m_instruction_queue.pop();
      }
      catch (DynamicInstructionInfoNotAvailableException)
      {
         LOG_PRINT("Throwing DynamicInstructionInfoNotAvailable!");
         return;
      }
   }
}

void CoreModel::pushDynamicInstructionInfo(DynamicInstructionInfo &i)
{
   if (!m_enabled)
      return;

   m_dynamic_info_queue.push(i);
}

void CoreModel::popDynamicInstructionInfo()
{
   if (!m_enabled)
      return;

   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() > 0,
                    "Expected some dynamic info to be available.");
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");
   LOG_PRINT("Pop Info(%u)", m_dynamic_info_queue.front().type);
   m_dynamic_info_queue.pop();
}

DynamicInstructionInfo& CoreModel::getDynamicInstructionInfo()
{
   // Information is needed to model the instruction, but isn't
   // available. This is handled in iterate() by returning early and
   // continuing from that instruction later.

   // FIXME: Note this assumes that either none of the info for an
   // instruction is available or all of it! This works for
   // performance modeling in the same thread as functional modeling.

   if (m_dynamic_info_queue.empty())
      throw DynamicInstructionInfoNotAvailableException();

   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");

   LOG_PRINT("Get Info(%u)", m_dynamic_info_queue.front().type);
   return m_dynamic_info_queue.front();
}
