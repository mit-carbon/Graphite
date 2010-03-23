#include "core.h"
#include "performance_model.h"
#include "branch_predictor.h"
#include "simulator.h"
#include "simple_performance_model.h"
#include "iocoom_performance_model.h"
#include "magic_performance_model.h"
#include "core_manager.h"

PerformanceModel* PerformanceModel::create(Core* core)
{
   string type;

   try {
      type = Sim()->getCfg()->getString("perf_model/core/type");
   } catch (...) {
      LOG_PRINT_ERROR("No perf model type provided.");
   }

   if (type == "iocoom")
      return new IOCOOMPerformanceModel(core);
   else if (type == "simple")
      return new SimplePerformanceModel(core);
   else if (type == "magic")
      return new MagicPerformanceModel(core);
   else
   {
      LOG_PRINT_ERROR("Invalid perf model type: %s", type.c_str());
      return NULL;
   }
}

// Public Interface
PerformanceModel::PerformanceModel(Core *core)
   : m_core(core)
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
