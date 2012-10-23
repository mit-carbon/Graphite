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
#include "utils.h"

CoreModel* CoreModel::create(Core* core)
{
   float frequency = Config::getSingleton()->getCoreFrequency(core->getId());
   string core_model = Config::getSingleton()->getCoreType(core->getTile()->getId());

   if (core_model == "iocoom")
      return new IOCOOMCoreModel(core, frequency);
   else if (core_model == "simple")
      return new SimpleCoreModel(core, frequency);
   else
   {
      LOG_PRINT_ERROR("Invalid core model type: %s", core_model.c_str());
      return NULL;
   }
}

// Public Interface
CoreModel::CoreModel(Core *core, float frequency)
   : m_cycle_count(0)
   , m_instruction_count(0)
   , m_frequency(frequency)
   , m_core(core)
   , m_average_frequency(0.0)
   , m_total_time(0)
   , m_checkpointed_cycle_count(0)
   , m_enabled(false)
   , m_current_ins_index(0)
   , m_bp(0)
{
   // Create Branch Predictor
   m_bp = BranchPredictor::create();

   // Initialize Pipeline Stall Counters
   initializePipelineStallCounters();
}

CoreModel::~CoreModel()
{
   delete m_bp; m_bp = 0;
}

void CoreModel::outputSummary(ostream& os)
{
   os << "Core Summary:" << endl;
   os << "    Total Instructions: " << m_instruction_count << endl;
   os << "    Completion Time (in ns): " << (UInt64) ((double) m_cycle_count / m_frequency) << endl;
   os << "    Average Frequency (in GHz): " << m_average_frequency << endl;
   
   os << "    Total Recv Instructions: " << m_total_recv_instructions << endl;
   os << "    Total Sync Instructions: " << m_total_sync_instructions << endl;
   os << "    Total Memory Stall Time (in ns): " << (UInt64) ((double) m_total_memory_stall_cycles / m_frequency) << endl;
   os << "    Total Execution Unit Stall Time (in ns): " << (UInt64) ((double) m_total_execution_unit_stall_cycles / m_frequency) << endl;
   os << "    Total Recv Instruction Stall Time (in ns): " << (UInt64) ((double) m_total_recv_instruction_stall_cycles / m_frequency) << endl;
   os << "    Total Sync Instruction Stall Time (in ns): " << (UInt64) ((double) m_total_sync_instruction_stall_cycles / m_frequency) << endl;

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
void CoreModel::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   recomputeAverageFrequency();
   
   volatile float old_frequency = m_frequency;
   volatile float new_frequency = frequency;
   
   m_checkpointed_cycle_count = (UInt64) (((double) m_cycle_count / old_frequency) * new_frequency);
   m_cycle_count = m_checkpointed_cycle_count;
   m_frequency = new_frequency;

   // Update Pipeline Stall Counters
   m_total_memory_stall_cycles = (UInt64) (((double) m_total_memory_stall_cycles / old_frequency) * new_frequency);
   m_total_execution_unit_stall_cycles = (UInt64) (((double) m_total_execution_unit_stall_cycles / old_frequency) * new_frequency);
   m_total_recv_instruction_stall_cycles = (UInt64) (((double) m_total_recv_instruction_stall_cycles / old_frequency) * new_frequency);
   m_total_sync_instruction_stall_cycles = (UInt64) (((double) m_total_sync_instruction_stall_cycles / old_frequency) * new_frequency);
}

// This function is called:
// 1) On thread start
void CoreModel::setCycleCount(UInt64 cycle_count)
{
   m_checkpointed_cycle_count = cycle_count;
   m_cycle_count = cycle_count;
}

// This function is called:
// 1) On thread exit
// 2) Whenever frequency is changed
void CoreModel::recomputeAverageFrequency()
{
   volatile double cycles_elapsed = (double) (m_cycle_count - m_checkpointed_cycle_count);
   volatile double total_cycles_executed = (m_average_frequency * m_total_time) + cycles_elapsed;
   volatile double total_time_taken = m_total_time + (cycles_elapsed / m_frequency);

   m_average_frequency = total_cycles_executed / total_time_taken;
   m_total_time = (UInt64) total_time_taken;
}

void CoreModel::initializePipelineStallCounters()
{
   m_total_recv_instructions = 0;
   m_total_sync_instructions = 0;
   m_total_recv_instruction_stall_cycles = 0;
   m_total_sync_instruction_stall_cycles = 0;
   m_total_memory_stall_cycles = 0;
   m_total_execution_unit_stall_cycles = 0;
}

void CoreModel::updatePipelineStallCounters(Instruction* i, UInt64 memory_stall_cycles, UInt64 execution_unit_stall_cycles)
{
   switch (i->getType())
   {
      case INST_RECV:
         m_total_recv_instructions ++;
         m_total_recv_instruction_stall_cycles += i->getCost();
         break;

      case INST_SYNC:
         m_total_sync_instructions ++;
         m_total_sync_instruction_stall_cycles += i->getCost();
         break;

      default:
         break;
   }
   
   m_total_memory_stall_cycles += memory_stall_cycles;
   m_total_execution_unit_stall_cycles += execution_unit_stall_cycles;
}

void CoreModel::queueDynamicInstruction(Instruction *i)
{
   if (!m_enabled)
   {
      delete i;
      return;
   }

   BasicBlock *bb = new BasicBlock(true);
   bb->push_back(i);
   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(bb);
}

void CoreModel::queueBasicBlock(BasicBlock *basic_block)
{
   if (!m_enabled)
      return;

   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(basic_block);
}

void CoreModel::iterate()
{
   // Because we will sometimes not have info available (we will throw
   // a DynamicInstructionInfoNotAvailable), we need to be able to
   // continue from the middle of a basic block. m_current_ins_index
   // tracks which instruction we are currently on within the basic
   // block.

   ScopedLock sl(m_basic_block_queue_lock);

   while (m_basic_block_queue.size() > 1)
   {
      LOG_PRINT("Basic Block Queue Size(%lu)", m_basic_block_queue.size());
      BasicBlock *current_bb = m_basic_block_queue.front();

      try
      {
         for( ; m_current_ins_index < current_bb->size(); m_current_ins_index++)
         {
            try
            {
               handleInstruction(current_bb->at(m_current_ins_index));
            }
            catch (AbortInstructionException)
            {
               // move on to next ...
            }
         }

         if (current_bb->isDynamic())
            delete current_bb;

         m_basic_block_queue.pop();
         m_current_ins_index = 0; // move to beginning of next bb
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

   ScopedLock sl(m_dynamic_info_queue_lock);
   m_dynamic_info_queue.push(i);
}

void CoreModel::popDynamicInstructionInfo()
{
   if (!m_enabled)
      return;

   ScopedLock sl(m_dynamic_info_queue_lock);
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() > 0,
                    "Expected some dynamic info to be available.");
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");
   LOG_PRINT("Pop Info(%u)", m_dynamic_info_queue.front().type);
   m_dynamic_info_queue.pop();
}

DynamicInstructionInfo& CoreModel::getDynamicInstructionInfo()
{
   ScopedLock sl(m_dynamic_info_queue_lock);

   // Information is needed to model the instruction, but isn't
   // available. This is handled in iterate() by returning early and
   // continuing from that instruction later.

   // FIXME: Note this assumes that either none of the info for an
   // instruction is available or all of it! This works for
   // performance modeling in the same thread as functional modeling,
   // but it WILL NOT work if performance modeling is moved to a
   // separate thread!

   if (m_dynamic_info_queue.empty())
      throw DynamicInstructionInfoNotAvailableException();

   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");

   LOG_PRINT("Get Info(%u)", m_dynamic_info_queue.front().type);
   return m_dynamic_info_queue.front();
}
