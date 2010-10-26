#include "performance_model.h"
#include "simple_performance_model.h"
#include "iocoom_performance_model.h"
#include "magic_performance_model.h"
#include "simulator.h"
#include "core_manager.h"
#include "config.h"
#include "core.h"
#include "branch_predictor.h"
#include "fxsupport.h"
#include "utils.h"

PerformanceModel* PerformanceModel::create(Tile* core)
{
   volatile float frequency = Config::getSingleton()->getCoreFrequency(core->getId());
   string core_model = Config::getSingleton()->getCoreType(core->getId());

   if (core_model == "iocoom")
      return new IOCOOMPerformanceModel(core, frequency);
   else if (core_model == "simple")
      return new SimplePerformanceModel(core, frequency);
   else if (core_model == "magic")
      return new MagicPerformanceModel(core, frequency);
   else
   {
      LOG_PRINT_ERROR("Invalid perf model type: %s", core_model.c_str());
      return NULL;
   }
}

// Public Interface
PerformanceModel::PerformanceModel(Tile *core, float frequency)
   : m_cycle_count(0)
   , m_core(core)
   , m_frequency(frequency)
   , m_average_frequency(0.0)
   , m_total_time(0)
   , m_checkpointed_cycle_count(0)
   , m_enabled(false)
   , m_current_ins_index(0)
   , m_bp(0)
{
   m_bp = BranchPredictor::create();
}

PerformanceModel::~PerformanceModel()
{
   delete m_bp; m_bp = 0;
}

void PerformanceModel::frequencySummary(ostream& os)
{
   os << "   Completion Time: " \
      << static_cast<UInt64>(static_cast<float>(m_cycle_count) / m_frequency) \
      << endl;
   os << "   Average Frequency: " << m_average_frequency << endl;
}

void PerformanceModel::enable()
{
   // MCP perf model should never be enabled
   if (Sim()->getCoreManager()->getCurrentCoreID() == Config::getSingleton()->getMCPCoreNum())
      return;

   m_enabled = true;
}

void PerformanceModel::disable()
{
   m_enabled = false;
}

// This function is called:
// 1) Whenever frequency is changed
void PerformanceModel::updateInternalVariablesOnFrequencyChange(volatile float frequency)
{
   recomputeAverageFrequency();
   m_frequency = frequency;
}

// This function is called:
// 1) On thread start
void PerformanceModel::setCycleCount(UInt64 cycle_count)
{
   m_checkpointed_cycle_count = cycle_count;
   m_cycle_count = cycle_count;
}

// This function is called:
// 1) On thread exit
// 2) Whenever frequency is changed
void PerformanceModel::recomputeAverageFrequency()
{
   volatile float cycles_elapsed = static_cast<float>(m_cycle_count - m_checkpointed_cycle_count);
   volatile float total_cycles_executed = m_average_frequency * m_total_time + cycles_elapsed;
   volatile float total_time_taken = m_total_time + cycles_elapsed / m_frequency;

   m_average_frequency = total_cycles_executed / total_time_taken;
   m_total_time = total_time_taken;
   m_checkpointed_cycle_count = m_cycle_count;
}

void PerformanceModel::queueDynamicInstruction(Instruction *i)
{
   if (!m_enabled || !Config::getSingleton()->getEnablePerformanceModeling())
   {
      delete i;
      return;
   }

   BasicBlock *bb = new BasicBlock(true);
   bb->push_back(i);
   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(bb);
}

void PerformanceModel::queueBasicBlock(BasicBlock *basic_block)
{
   if (!m_enabled || !Config::getSingleton()->getEnablePerformanceModeling())
      return;

   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(basic_block);
}

//FIXME: this will go in a thread
void PerformanceModel::iterate()
{
   // Because we will sometimes not have info available (we will throw
   // a DynamicInstructionInfoNotAvailable), we need to be able to
   // continue from the middle of a basic block. m_current_ins_index
   // tracks which instruction we are currently on within the basic
   // block.

   ScopedLock sl(m_basic_block_queue_lock);

   while (m_basic_block_queue.size() > 1)
   {
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
         return;
      }
   }
}

void PerformanceModel::pushDynamicInstructionInfo(DynamicInstructionInfo &i)
{
   if (!m_enabled || !Config::getSingleton()->getEnablePerformanceModeling())
      return;

   ScopedLock sl(m_dynamic_info_queue_lock);
   m_dynamic_info_queue.push(i);
}

void PerformanceModel::popDynamicInstructionInfo()
{
   if (!m_enabled || !Config::getSingleton()->getEnablePerformanceModeling())
      return;

   ScopedLock sl(m_dynamic_info_queue_lock);
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() > 0,
                    "Expected some dynamic info to be available.");
   LOG_ASSERT_ERROR(m_dynamic_info_queue.size() < 5000,
                    "Dynamic info queue is growing too big.");
   m_dynamic_info_queue.pop();
}

DynamicInstructionInfo& PerformanceModel::getDynamicInstructionInfo()
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

   return m_dynamic_info_queue.front();
}
