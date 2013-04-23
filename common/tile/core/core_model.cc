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
#include "mcpat_core_interface.h"

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
   , m_current_ins_index(0)
   , m_bp(0)
{
   // Create Branch Predictor
   m_bp = BranchPredictor::create();

   // Initialize Pipeline Stall Counters
   initializePipelineStallCounters();

   // Initialize instruction costs
   initializeCoreStaticInstructionModel(core->getFrequency());
   
   // For Power/Area Modeling
   m_enable_area_or_power_modeling = Config::getSingleton()->getEnableAreaModeling() || Config::getSingleton()->getEnablePowerModeling();
   
   LOG_PRINT("Initialized CoreModel.");
}

CoreModel::~CoreModel()
{
   delete m_mcpat_core_interface;
   delete m_bp; m_bp = 0;
}


void CoreModel::initializeCoreStaticInstructionModel(volatile double frequency)
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

void CoreModel::updateCoreStaticInstructionModel(double frequency)
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
   
   // McPAT summary
   m_mcpat_core_interface->displayStats(os);
   m_mcpat_core_interface->displayParam(os);
   
   if (m_enable_area_or_power_modeling)
   {
      os << "  Area and Power Model Summary:" << endl;
      // Compute Energy for Total Run
      m_mcpat_core_interface->computeEnergy();
      m_mcpat_core_interface->displayEnergy(os);
   }
}

void CoreModel::initializeMcPATInterface(UInt32 num_load_buffer_entries, UInt32 num_store_buffer_entries)
{
   // For Power/Area Modeling
   double frequency = m_core->getFrequency();
   double voltage = m_core->getVoltage();
   m_mcpat_core_interface = new McPATCoreInterface(frequency, voltage, num_load_buffer_entries, num_store_buffer_entries);
}

void CoreModel::updateMcPATCounters(Instruction* instruction)
{
   // Get Branch Misprediction Count
   UInt64 total_branch_misprediction_count = m_bp->getNumIncorrectPredictions();

   // Update Event Counters
   m_mcpat_core_interface->updateEventCounters(instruction, m_curr_time.toCycles(m_core->getFrequency()), total_branch_misprediction_count);
}

void CoreModel::computeEnergy()
{
   m_mcpat_core_interface->updateCycleCounters(m_curr_time.toCycles(m_core->getFrequency()));
   m_mcpat_core_interface->computeEnergy();
}

double CoreModel::getDynamicEnergy()
{
   double dynamic_energy = m_mcpat_core_interface->getDynamicEnergy();
   return dynamic_energy;
}

double CoreModel::getStaticPower()
{
   double static_power = m_mcpat_core_interface->getStaticPower();
   return static_power;
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
void CoreModel::setDVFS(double old_frequency, double new_voltage, double new_frequency)
{
   recomputeAverageFrequency(old_frequency);
   updateCoreStaticInstructionModel(new_frequency);
   m_mcpat_core_interface->setDVFS(new_voltage, new_frequency);
}

void CoreModel::setCurrTime(Time time)
{
   m_curr_time = time;
   m_checkpointed_time = time;
}

// This function is called:
// 1) On thread exit
// 2) Whenever frequency is changed
void CoreModel::recomputeAverageFrequency(double old_frequency)
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

void CoreModel::processDynamicInstruction(Instruction* i)
{
   bool queued = queueDynamicInstruction(i);
   if (queued)
      iterate();
}

bool CoreModel::queueDynamicInstruction(Instruction *i)
{
   if (!m_enabled)
   {
      delete i;
      return false;
   }

   BasicBlock *bb = new BasicBlock(true);
   bb->push_back(i);
   ScopedLock sl(m_basic_block_queue_lock);
   m_basic_block_queue.push(bb);
   return true;
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
