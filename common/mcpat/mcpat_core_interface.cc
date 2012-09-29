/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "mcpat_core_interface.h"
#include "config.h"
#include "log.h"

using namespace std;

//---------------------------------------------------------------------------
// McPAT Core Interface Constructor
//---------------------------------------------------------------------------
McPATCoreInterface::McPATCoreInterface(UInt32 technology_node, UInt32 core_frequency, UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // Initialize Architectural Parameters
   initializeArchitecturalParameters(technology_node, core_frequency, load_buffer_size, store_buffer_size);
   // Initialize Event Counters
   initializeEventCounters();

   m_enable_area_and_power_modeling = Config::getSingleton()->getEnableAreaModeling() || Config::getSingleton()->getEnablePowerModeling();
   if (m_enable_area_and_power_modeling)
   {
      // Initialize Output Data Structure
      initializeOutputDataStructure();
      
      // Make a ParseXML Object and Initialize it
      mcpat_parsexml = new McPAT::ParseXML();

      // Initialize ParseXML Params and Stats
      mcpat_parsexml->initialize();
      mcpat_parsexml->setNiagara1();

      // Fill the ParseXML's Core Params from McPATCoreInterface
      fillCoreParamsIntoXML();

      // Make a Processor Object from the ParseXML
      mcpat_core = new McPAT::CoreWrapper(mcpat_parsexml);
   }
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCoreInterface::~McPATCoreInterface()
{
   if (m_enable_area_and_power_modeling)
   {
      delete mcpat_parsexml;
      delete mcpat_core;
   }
}

//---------------------------------------------------------------------------
// Initialize Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeArchitecturalParameters(UInt32 technology_node, UInt32 core_frequency, UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // System Parameters
   // |---- General Parameters
   m_clock_rate = core_frequency;
   m_core_tech_node = technology_node;
   // Architectural Parameters
   // |---- General Parameters
   m_instruction_length = 32;
   m_opcode_width = 9;
   m_machine_type = 1;
   m_num_hardware_threads = 1;
   m_fetch_width = 1;
   m_num_instruction_fetch_ports = 1;
   m_decode_width = 1;
   m_issue_width = 1;
   m_fp_issue_width = 1;
   m_commit_width = 1;
   m_prediction_width = 0;
   m_integer_pipeline_depth = 6;
   m_fp_pipeline_depth = 6;
   m_ALU_per_core = 1;
   m_MUL_per_core = 1;
   m_FPU_per_core = 1;
   m_instruction_buffer_size = 16;
   m_decoded_stream_buffer_size = 16;
   // |---- Register File
   m_arch_regs_IRF_size = 24;
   m_arch_regs_FRF_size = 24;
   m_phy_regs_IRF_size = 24;
   m_phy_regs_FRF_size = 24;
   // |---- Load-Store Unit
   m_LSU_order = "inorder";
   m_load_buffer_size = load_buffer_size;
   m_store_buffer_size = store_buffer_size;
   m_num_memory_ports = 1;
   m_RAS_size = 16;
   // |---- OoO Core
   m_instruction_window_scheme = 0;
   m_instruction_window_size = 0;
   m_fp_instruction_window_size = 0;
   m_ROB_size = 0;
   m_rename_scheme = 0;
   // |---- Register Windows (specific to Sun processors)
   m_register_windows_size = 0;
}

//---------------------------------------------------------------------------
// Initialize Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeEventCounters()
{
   // Event Counters
   m_execution_time = 0;
   // |-- Used Event Counters
   // |---- Instruction Counters
   m_total_instructions = 0;
   m_int_instructions = 0;
   m_fp_instructions = 0;
   m_branch_instructions = 0;
   m_branch_mispredictions = 0;
   m_load_instructions = 0;
   m_store_instructions = 0;
   m_committed_instructions = 0;
   m_committed_int_instructions = 0;
   m_committed_fp_instructions = 0;
   // |---- Cycle Counters
   m_total_cycles = 0;
   m_idle_cycles = 0;
   m_busy_cycles = 0;
   // |---- Reg File Access Counters
   m_int_regfile_reads = 0;
   m_int_regfile_writes = 0;
   m_fp_regfile_reads = 0;
   m_fp_regfile_writes = 0;
   // |---- Execution Unit Access Counters
   m_ialu_accesses = 0;
   m_mul_accesses = 0;
   m_fpu_accesses = 0;
   m_cdb_alu_accesses = 0;
   m_cdb_mul_accesses = 0;
   m_cdb_fpu_accesses = 0;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   m_inst_window_reads = 0;
   m_inst_window_writes = 0;
   m_inst_window_wakeup_accesses = 0;
   m_fp_inst_window_reads = 0;
   m_fp_inst_window_writes = 0;
   m_fp_inst_window_wakeup_accesses = 0;
   m_ROB_reads = 0;
   m_ROB_writes = 0;
   m_rename_accesses = 0;
   m_fp_rename_accesses = 0;
   // |---- Function Calls and Context Switches
   m_function_calls = 0;
   m_context_switches = 0;
}

//---------------------------------------------------------------------------
// Initialize Output Data Structure
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeOutputDataStructure()
{
   // Zero the Energy in Data Structure
   // Core
   mcpat_core_out.core.area                           = 0;
   mcpat_core_out.core.leakage                        = 0;
   mcpat_core_out.core.longer_channel_leakage         = 0;
   mcpat_core_out.core.gate_leakage                   = 0;
   mcpat_core_out.core.dynamic                        = 0;
   //    Instruction Fetch Unit
   mcpat_core_out.ifu.ifu.area                        = 0;
   mcpat_core_out.ifu.ifu.leakage                     = 0;
   mcpat_core_out.ifu.ifu.longer_channel_leakage      = 0;
   mcpat_core_out.ifu.ifu.gate_leakage                = 0;
   mcpat_core_out.ifu.ifu.dynamic                     = 0;
   //       Instruction Cache
   mcpat_core_out.ifu.icache.area                     = 0;
   mcpat_core_out.ifu.icache.leakage                  = 0;
   mcpat_core_out.ifu.icache.longer_channel_leakage   = 0;
   mcpat_core_out.ifu.icache.gate_leakage             = 0;
   mcpat_core_out.ifu.icache.dynamic                  = 0;
   //       Instruction Buffer
   mcpat_core_out.ifu.IB.area                         = 0;
   mcpat_core_out.ifu.IB.leakage                      = 0;
   mcpat_core_out.ifu.IB.longer_channel_leakage       = 0;
   mcpat_core_out.ifu.IB.gate_leakage                 = 0;
   mcpat_core_out.ifu.IB.dynamic                      = 0;
   //       Instruction Decoder
   mcpat_core_out.ifu.ID.area                         = 0;
   mcpat_core_out.ifu.ID.leakage                      = 0;
   mcpat_core_out.ifu.ID.longer_channel_leakage       = 0;
   mcpat_core_out.ifu.ID.gate_leakage                 = 0;
   mcpat_core_out.ifu.ID.dynamic                      = 0;
   //    Load Store Unit
   mcpat_core_out.lsu.lsu.area                        = 0;
   mcpat_core_out.lsu.lsu.leakage                     = 0;
   mcpat_core_out.lsu.lsu.longer_channel_leakage      = 0;
   mcpat_core_out.lsu.lsu.gate_leakage                = 0;
   mcpat_core_out.lsu.lsu.dynamic                     = 0;
   //       Data Cache
   mcpat_core_out.lsu.dcache.area                     = 0;
   mcpat_core_out.lsu.dcache.leakage                  = 0;
   mcpat_core_out.lsu.dcache.longer_channel_leakage   = 0;
   mcpat_core_out.lsu.dcache.gate_leakage             = 0;
   mcpat_core_out.lsu.dcache.dynamic                  = 0;
   //       Load/Store Queue
   mcpat_core_out.lsu.LSQ.area                        = 0;
   mcpat_core_out.lsu.LSQ.leakage                     = 0;
   mcpat_core_out.lsu.LSQ.longer_channel_leakage      = 0;
   mcpat_core_out.lsu.LSQ.gate_leakage                = 0;
   mcpat_core_out.lsu.LSQ.dynamic                     = 0;
   //    Memory Management Unit
   mcpat_core_out.mmu.mmu.area                        = 0;
   mcpat_core_out.mmu.mmu.leakage                     = 0;
   mcpat_core_out.mmu.mmu.longer_channel_leakage      = 0;
   mcpat_core_out.mmu.mmu.gate_leakage                = 0;
   mcpat_core_out.mmu.mmu.dynamic                     = 0;
   //       Itlb
   mcpat_core_out.mmu.itlb.area                       = 0;
   mcpat_core_out.mmu.itlb.leakage                    = 0;
   mcpat_core_out.mmu.itlb.longer_channel_leakage     = 0;
   mcpat_core_out.mmu.itlb.gate_leakage               = 0;
   mcpat_core_out.mmu.itlb.dynamic                    = 0;
   //       Dtlb
   mcpat_core_out.mmu.dtlb.area                       = 0;
   mcpat_core_out.mmu.dtlb.leakage                    = 0;
   mcpat_core_out.mmu.dtlb.longer_channel_leakage     = 0;
   mcpat_core_out.mmu.dtlb.gate_leakage               = 0;
   mcpat_core_out.mmu.dtlb.dynamic                    = 0;
   //    Execution Unit
   mcpat_core_out.exu.exu.area                        = 0;
   mcpat_core_out.exu.exu.leakage                     = 0;
   mcpat_core_out.exu.exu.longer_channel_leakage      = 0;
   mcpat_core_out.exu.exu.gate_leakage                = 0;
   mcpat_core_out.exu.exu.dynamic                     = 0;
   //       Register Files
   mcpat_core_out.exu.rfu.rfu.area                    = 0;
   mcpat_core_out.exu.rfu.rfu.leakage                 = 0;
   mcpat_core_out.exu.rfu.rfu.longer_channel_leakage  = 0;
   mcpat_core_out.exu.rfu.rfu.gate_leakage            = 0;
   mcpat_core_out.exu.rfu.rfu.dynamic                 = 0;
   //          Integer RF
   mcpat_core_out.exu.rfu.IRF.area                    = 0;
   mcpat_core_out.exu.rfu.IRF.leakage                 = 0;
   mcpat_core_out.exu.rfu.IRF.longer_channel_leakage  = 0;
   mcpat_core_out.exu.rfu.IRF.gate_leakage            = 0;
   mcpat_core_out.exu.rfu.IRF.dynamic                 = 0;
   //          Floating Point RF
   mcpat_core_out.exu.rfu.FRF.area                    = 0;
   mcpat_core_out.exu.rfu.FRF.leakage                 = 0;
   mcpat_core_out.exu.rfu.FRF.longer_channel_leakage  = 0;
   mcpat_core_out.exu.rfu.FRF.gate_leakage            = 0;
   mcpat_core_out.exu.rfu.FRF.dynamic                 = 0;
   //          Register Windows
   mcpat_core_out.exu.rfu.RFWIN.area                     = 0;
   mcpat_core_out.exu.rfu.RFWIN.leakage                  = 0;
   mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage   = 0;
   mcpat_core_out.exu.rfu.RFWIN.gate_leakage             = 0;
   mcpat_core_out.exu.rfu.RFWIN.dynamic                  = 0;
   //       Instruction Scheduler
   mcpat_core_out.exu.scheu.scheu.area                   = 0;
   mcpat_core_out.exu.scheu.scheu.leakage                = 0;
   mcpat_core_out.exu.scheu.scheu.longer_channel_leakage = 0;
   mcpat_core_out.exu.scheu.scheu.gate_leakage           = 0;
   mcpat_core_out.exu.scheu.scheu.dynamic                = 0;
   //          Instruction Window
   mcpat_core_out.exu.scheu.int_inst_window.area                     = 0;
   mcpat_core_out.exu.scheu.int_inst_window.leakage                  = 0;
   mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage   = 0;
   mcpat_core_out.exu.scheu.int_inst_window.gate_leakage             = 0;
   mcpat_core_out.exu.scheu.int_inst_window.dynamic                  = 0;
   //       Integer ALUs
   mcpat_core_out.exu.exeu.area                       = 0;
   mcpat_core_out.exu.exeu.leakage                    = 0;
   mcpat_core_out.exu.exeu.longer_channel_leakage     = 0;
   mcpat_core_out.exu.exeu.gate_leakage               = 0;
   mcpat_core_out.exu.exeu.dynamic                    = 0;
   //       Floating Point Units (FPUs)
   mcpat_core_out.exu.fp_u.area                       = 0;
   mcpat_core_out.exu.fp_u.leakage                    = 0;
   mcpat_core_out.exu.fp_u.longer_channel_leakage     = 0;
   mcpat_core_out.exu.fp_u.gate_leakage               = 0;
   mcpat_core_out.exu.fp_u.dynamic                    = 0;
   //       Complex ALUs (Mul/Div)
   mcpat_core_out.exu.mul.area                        = 0;
   mcpat_core_out.exu.mul.leakage                     = 0;
   mcpat_core_out.exu.mul.longer_channel_leakage      = 0;
   mcpat_core_out.exu.mul.gate_leakage                = 0;
   mcpat_core_out.exu.mul.dynamic                     = 0;
   //       Results Broadcast Bus
   mcpat_core_out.exu.bypass.area                     = 0;
   mcpat_core_out.exu.bypass.leakage                  = 0;
   mcpat_core_out.exu.bypass.longer_channel_leakage   = 0;
   mcpat_core_out.exu.bypass.gate_leakage             = 0;
   mcpat_core_out.exu.bypass.dynamic                  = 0;
}

void McPATCoreInterface::updateEventCounters(Instruction* instruction, UInt64 cycle_count)
{
   // Get Instruction Type
   McPATInstructionType instruction_type = getMcPATInstructionType(instruction->getType());
   updateInstructionCounters(instruction_type);

   // Update Cycle Counters
   updateCycleCounters(cycle_count);
   
   const OperandList& ops = instruction->getOperands();
   for (unsigned int i = 0; i < ops.size(); i++)
   {
      const Operand &o = ops[i];

      // Loads/Stores
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::READ))
         updateInstructionCounters(LOAD_INST);
      if ((o.m_type == Operand::MEMORY) && (o.m_direction == Operand::WRITE))
         updateInstructionCounters(STORE_INST);

      // Reg File Accesses
      if (o.m_type == Operand::REG)
         updateRegFileAccessCounters(o.m_direction, o.m_value);
   }

   // Execution Unit Accesses
   // A single instruction can access multiple execution units
   // FIXME: Find out whether we need the whole instruction for this purpose
   ExecutionUnitList access_list = getExecutionUnitAccessList(instruction->getType());
   for (UInt32 i = 0; i < access_list.size(); i++)
      updateExecutionUnitAccessCounters(access_list[i]);
}

void McPATCoreInterface::updateInstructionCounters(McPATInstructionType instruction_type)
{
   m_total_instructions ++;
   m_committed_instructions ++;
   
   switch (instruction_type)
   {
   case GENERIC_INST:
      break;

   case INTEGER_INST:
      m_int_instructions ++;
      m_committed_int_instructions ++;
      break;

   case FLOATING_POINT_INST:
      m_fp_instructions ++;
      m_committed_fp_instructions ++;
      break;

   case LOAD_INST:
      m_load_instructions ++;
      break;

   case STORE_INST:
      m_store_instructions ++;
      break;

   case BRANCH_INST:
      m_branch_instructions ++;
      break;

   case BRANCH_NOT_TAKEN_INST:
      m_branch_mispredictions ++;
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Instruction Type(%u)", instruction_type);
      break;
   }
}

void McPATCoreInterface::updateRegFileAccessCounters(Operand::Direction operand_direction, UInt32 reg_id)
{
   if (operand_direction == Operand::READ)
   {
      if (isIntegerReg(reg_id))
         m_int_regfile_reads ++;
      else if (isFloatingPointReg(reg_id))
         m_fp_regfile_reads ++;
      else if (isXMMReg(reg_id))
         m_fp_regfile_reads += 2;
   }
   else if (operand_direction == Operand::WRITE)
   {
      if (isIntegerReg(reg_id))
         m_int_regfile_writes ++;
      else if (isFloatingPointReg(reg_id))
         m_fp_regfile_writes ++;
      else if (isXMMReg(reg_id))
         m_fp_regfile_writes += 2;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Operand Direction(%u)", operand_direction);
   }
}

void McPATCoreInterface::updateExecutionUnitAccessCounters(ExecutionUnitType unit_type)
{
   switch (unit_type)
   {
   case ALU:
      m_ialu_accesses ++;
      m_cdb_alu_accesses ++;
      break;

   case MUL:
      m_mul_accesses ++;
      m_cdb_mul_accesses ++;
      break;

   case FPU:
      m_fpu_accesses ++;
      m_cdb_fpu_accesses ++;
      break;

   default:
      LOG_PRINT_ERROR("Unrecognized Execution Unit(%u)", unit_type);
      break;
   }
}

void McPATCoreInterface::updateCycleCounters(UInt64 cycle_count)
{
   m_total_cycles = cycle_count;
   m_busy_cycles = cycle_count;
   // TODO: Update for idle cycles later
}

//---------------------------------------------------------------------------
// Compute Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::computeMcPATCoreEnergy()
{
   // Fill the ParseXML's Core Event Stats from McPATCoreInterface
   fillCoreStatsIntoXML();

   // Compute Energy from Processor
   mcpat_core->computeEnergy();

   // Execution Time
   m_execution_time = (mcpat_core->cores[0]->coredynp.executionTime);
   //cout << "m_execution_time = " << m_execution_time << endl;

   // Store Energy into Data Structure
   // Core
   mcpat_core_out.core.area                   = mcpat_core->cores[0]->area.get_area()*1e-6;                     
   mcpat_core_out.core.leakage                = mcpat_core->cores[0]->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.core.longer_channel_leakage = mcpat_core->cores[0]->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.core.gate_leakage           = mcpat_core->cores[0]->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.core.dynamic                = mcpat_core->cores[0]->rt_power.readOp.dynamic;
   //    Instruction Fetch Unit
   mcpat_core_out.ifu.ifu.area                   = mcpat_core->cores[0]->ifu->area.get_area()*1e-6;                     
   mcpat_core_out.ifu.ifu.leakage                = mcpat_core->cores[0]->ifu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.ifu.ifu.longer_channel_leakage = mcpat_core->cores[0]->ifu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.ifu.ifu.gate_leakage           = mcpat_core->cores[0]->ifu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.ifu.ifu.dynamic                = mcpat_core->cores[0]->ifu->rt_power.readOp.dynamic;
   //       Instruction Cache
   mcpat_core_out.ifu.icache.area                   = mcpat_core->cores[0]->ifu->icache.area.get_area()*1e-6;                     
   mcpat_core_out.ifu.icache.leakage                = mcpat_core->cores[0]->ifu->icache.power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.ifu.icache.longer_channel_leakage = mcpat_core->cores[0]->ifu->icache.power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.ifu.icache.gate_leakage           = mcpat_core->cores[0]->ifu->icache.power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.ifu.icache.dynamic                = mcpat_core->cores[0]->ifu->icache.rt_power.readOp.dynamic;
   //       Instruction Buffer
   mcpat_core_out.ifu.IB.area                       = mcpat_core->cores[0]->ifu->IB->area.get_area()*1e-6;                     
   mcpat_core_out.ifu.IB.leakage                    = mcpat_core->cores[0]->ifu->IB->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.ifu.IB.longer_channel_leakage     = mcpat_core->cores[0]->ifu->IB->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.ifu.IB.gate_leakage               = mcpat_core->cores[0]->ifu->IB->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.ifu.IB.dynamic                    = mcpat_core->cores[0]->ifu->IB->rt_power.readOp.dynamic;
   //       Instruction Decoder
   mcpat_core_out.ifu.ID.area                       = (mcpat_core->cores[0]->ifu->ID_inst->area.get_area() +
         mcpat_core->cores[0]->ifu->ID_operand->area.get_area() +
         mcpat_core->cores[0]->ifu->ID_misc->area.get_area())*
         mcpat_core->cores[0]->ifu->coredynp.decodeW*1e-6;                     
   mcpat_core_out.ifu.ID.leakage                    = (mcpat_core->cores[0]->ifu->ID_inst->power.readOp.leakage +
         mcpat_core->cores[0]->ifu->ID_operand->power.readOp.leakage +
         mcpat_core->cores[0]->ifu->ID_misc->power.readOp.leakage);                
   mcpat_core_out.ifu.ID.longer_channel_leakage     = (mcpat_core->cores[0]->ifu->ID_inst->power.readOp.longer_channel_leakage +
         mcpat_core->cores[0]->ifu->ID_operand->power.readOp.longer_channel_leakage +
         mcpat_core->cores[0]->ifu->ID_misc->power.readOp.longer_channel_leakage);   
   mcpat_core_out.ifu.ID.gate_leakage               = (mcpat_core->cores[0]->ifu->ID_inst->power.readOp.gate_leakage +
         mcpat_core->cores[0]->ifu->ID_operand->power.readOp.gate_leakage +
         mcpat_core->cores[0]->ifu->ID_misc->power.readOp.gate_leakage);          
   mcpat_core_out.ifu.ID.dynamic                    = (mcpat_core->cores[0]->ifu->ID_inst->rt_power.readOp.dynamic +
         mcpat_core->cores[0]->ifu->ID_operand->rt_power.readOp.dynamic +
         mcpat_core->cores[0]->ifu->ID_misc->rt_power.readOp.dynamic);
   //    Load Store Unit
   mcpat_core_out.lsu.lsu.area                   = mcpat_core->cores[0]->lsu->area.get_area()*1e-6;                     
   mcpat_core_out.lsu.lsu.leakage                = mcpat_core->cores[0]->lsu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.lsu.lsu.longer_channel_leakage = mcpat_core->cores[0]->lsu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.lsu.lsu.gate_leakage           = mcpat_core->cores[0]->lsu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.lsu.lsu.dynamic                = mcpat_core->cores[0]->lsu->rt_power.readOp.dynamic;
   //       Data Cache
   mcpat_core_out.lsu.dcache.area                   = mcpat_core->cores[0]->lsu->dcache.area.get_area()*1e-6;                     
   mcpat_core_out.lsu.dcache.leakage                = mcpat_core->cores[0]->lsu->dcache.power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.lsu.dcache.longer_channel_leakage = mcpat_core->cores[0]->lsu->dcache.power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.lsu.dcache.gate_leakage           = mcpat_core->cores[0]->lsu->dcache.power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.lsu.dcache.dynamic                = mcpat_core->cores[0]->lsu->dcache.rt_power.readOp.dynamic;
   //       Load/Store Queue
   mcpat_core_out.lsu.LSQ.area                      = mcpat_core->cores[0]->lsu->LSQ->area.get_area()*1e-6;                     
   mcpat_core_out.lsu.LSQ.leakage                   = mcpat_core->cores[0]->lsu->LSQ->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.lsu.LSQ.longer_channel_leakage    = mcpat_core->cores[0]->lsu->LSQ->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.lsu.LSQ.gate_leakage              = mcpat_core->cores[0]->lsu->LSQ->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.lsu.LSQ.dynamic                   = mcpat_core->cores[0]->lsu->LSQ->rt_power.readOp.dynamic;
   //    Memory Management Unit
   mcpat_core_out.mmu.mmu.area                   = mcpat_core->cores[0]->mmu->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.mmu.leakage                = mcpat_core->cores[0]->mmu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.mmu.mmu.longer_channel_leakage = mcpat_core->cores[0]->mmu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.mmu.mmu.gate_leakage           = mcpat_core->cores[0]->mmu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.mmu.mmu.dynamic                = mcpat_core->cores[0]->mmu->rt_power.readOp.dynamic;
   //       Itlb
   mcpat_core_out.mmu.itlb.area                     = mcpat_core->cores[0]->mmu->itlb->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.itlb.leakage                  = mcpat_core->cores[0]->mmu->itlb->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.mmu.itlb.longer_channel_leakage   = mcpat_core->cores[0]->mmu->itlb->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.mmu.itlb.gate_leakage             = mcpat_core->cores[0]->mmu->itlb->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.mmu.itlb.dynamic                  = mcpat_core->cores[0]->mmu->itlb->rt_power.readOp.dynamic;
   //       Dtlb
   mcpat_core_out.mmu.dtlb.area                     = mcpat_core->cores[0]->mmu->dtlb->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.dtlb.leakage                  = mcpat_core->cores[0]->mmu->dtlb->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.mmu.dtlb.longer_channel_leakage   = mcpat_core->cores[0]->mmu->dtlb->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.mmu.dtlb.gate_leakage             = mcpat_core->cores[0]->mmu->dtlb->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.mmu.dtlb.dynamic                  = mcpat_core->cores[0]->mmu->dtlb->rt_power.readOp.dynamic;
   //    Execution Unit
   mcpat_core_out.exu.exu.area                   = mcpat_core->cores[0]->exu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.exu.leakage                = mcpat_core->cores[0]->exu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.exu.longer_channel_leakage = mcpat_core->cores[0]->exu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.exu.gate_leakage           = mcpat_core->cores[0]->exu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.exu.dynamic                = mcpat_core->cores[0]->exu->rt_power.readOp.dynamic;
   //       Register Files
   mcpat_core_out.exu.rfu.rfu.area                        = mcpat_core->cores[0]->exu->rfu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.rfu.leakage                     = mcpat_core->cores[0]->exu->rfu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.rfu.rfu.longer_channel_leakage      = mcpat_core->cores[0]->exu->rfu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.rfu.rfu.gate_leakage                = mcpat_core->cores[0]->exu->rfu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.rfu.rfu.dynamic                     = mcpat_core->cores[0]->exu->rfu->rt_power.readOp.dynamic;
   //          Integer RF
   mcpat_core_out.exu.rfu.IRF.area                           = mcpat_core->cores[0]->exu->rfu->IRF->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.IRF.leakage                        = mcpat_core->cores[0]->exu->rfu->IRF->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.rfu.IRF.longer_channel_leakage         = mcpat_core->cores[0]->exu->rfu->IRF->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.rfu.IRF.gate_leakage                   = mcpat_core->cores[0]->exu->rfu->IRF->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.rfu.IRF.dynamic                        = mcpat_core->cores[0]->exu->rfu->IRF->rt_power.readOp.dynamic;
   //          Floating Point RF
   mcpat_core_out.exu.rfu.FRF.area                           = mcpat_core->cores[0]->exu->rfu->FRF->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.FRF.leakage                        = mcpat_core->cores[0]->exu->rfu->FRF->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.rfu.FRF.longer_channel_leakage         = mcpat_core->cores[0]->exu->rfu->FRF->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.rfu.FRF.gate_leakage                   = mcpat_core->cores[0]->exu->rfu->FRF->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.rfu.FRF.dynamic                        = mcpat_core->cores[0]->exu->rfu->FRF->rt_power.readOp.dynamic;
   //          Register Windows
   if (mcpat_core->cores[0]->exu->rfu->RFWIN)
   {
      mcpat_core_out.exu.rfu.RFWIN.area                         = mcpat_core->cores[0]->exu->rfu->RFWIN->area.get_area()*1e-6;                     
      mcpat_core_out.exu.rfu.RFWIN.leakage                      = mcpat_core->cores[0]->exu->rfu->RFWIN->power.readOp.leakage*m_execution_time;                  
      mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage       = mcpat_core->cores[0]->exu->rfu->RFWIN->power.readOp.longer_channel_leakage*m_execution_time;   
      mcpat_core_out.exu.rfu.RFWIN.gate_leakage                 = mcpat_core->cores[0]->exu->rfu->RFWIN->power.readOp.gate_leakage*m_execution_time;             
      mcpat_core_out.exu.rfu.RFWIN.dynamic                      = mcpat_core->cores[0]->exu->rfu->RFWIN->rt_power.readOp.dynamic;
   }
   //       Instruction Scheduler
   mcpat_core_out.exu.scheu.scheu.area                    = mcpat_core->cores[0]->exu->scheu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.scheu.scheu.leakage                 = mcpat_core->cores[0]->exu->scheu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.scheu.scheu.longer_channel_leakage  = mcpat_core->cores[0]->exu->scheu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.scheu.scheu.gate_leakage            = mcpat_core->cores[0]->exu->scheu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.scheu.scheu.dynamic                 = mcpat_core->cores[0]->exu->scheu->rt_power.readOp.dynamic;
   //          Instruction Window
   if (mcpat_core->cores[0]->exu->scheu->int_inst_window)
   {
      mcpat_core_out.exu.scheu.int_inst_window.area                    = mcpat_core->cores[0]->exu->scheu->int_inst_window->area.get_area()*1e-6;                     
      mcpat_core_out.exu.scheu.int_inst_window.leakage                 = mcpat_core->cores[0]->exu->scheu->int_inst_window->power.readOp.leakage*m_execution_time;                  
      mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage  = mcpat_core->cores[0]->exu->scheu->int_inst_window->power.readOp.longer_channel_leakage*m_execution_time;   
      mcpat_core_out.exu.scheu.int_inst_window.gate_leakage            = mcpat_core->cores[0]->exu->scheu->int_inst_window->power.readOp.gate_leakage*m_execution_time;             
      mcpat_core_out.exu.scheu.int_inst_window.dynamic                 = mcpat_core->cores[0]->exu->scheu->int_inst_window->rt_power.readOp.dynamic;
   }
   //       Integer ALUs
   mcpat_core_out.exu.exeu.area                     = mcpat_core->cores[0]->exu->exeu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.exeu.leakage                  = mcpat_core->cores[0]->exu->exeu->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.exeu.longer_channel_leakage   = mcpat_core->cores[0]->exu->exeu->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.exeu.gate_leakage             = mcpat_core->cores[0]->exu->exeu->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.exeu.dynamic                  = mcpat_core->cores[0]->exu->exeu->rt_power.readOp.dynamic;
   //       Floating Point Units (FPUs)
   mcpat_core_out.exu.fp_u.area                     = mcpat_core->cores[0]->exu->fp_u->area.get_area()*1e-6;                     
   mcpat_core_out.exu.fp_u.leakage                  = mcpat_core->cores[0]->exu->fp_u->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.fp_u.longer_channel_leakage   = mcpat_core->cores[0]->exu->fp_u->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.fp_u.gate_leakage             = mcpat_core->cores[0]->exu->fp_u->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.fp_u.dynamic                  = mcpat_core->cores[0]->exu->fp_u->rt_power.readOp.dynamic;
   //       Complex ALUs (Mul/Div)
   mcpat_core_out.exu.mul.area                      = mcpat_core->cores[0]->exu->mul->area.get_area()*1e-6;                     
   mcpat_core_out.exu.mul.leakage                   = mcpat_core->cores[0]->exu->mul->power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.mul.longer_channel_leakage    = mcpat_core->cores[0]->exu->mul->power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.mul.gate_leakage              = mcpat_core->cores[0]->exu->mul->power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.mul.dynamic                   = mcpat_core->cores[0]->exu->mul->rt_power.readOp.dynamic;
   //       Results Broadcast Bus
   mcpat_core_out.exu.bypass.area                   = mcpat_core->cores[0]->exu->bypass.area.get_area()*1e-6;                     
   mcpat_core_out.exu.bypass.leakage                = mcpat_core->cores[0]->exu->bypass.power.readOp.leakage*m_execution_time;                  
   mcpat_core_out.exu.bypass.longer_channel_leakage = mcpat_core->cores[0]->exu->bypass.power.readOp.longer_channel_leakage*m_execution_time;   
   mcpat_core_out.exu.bypass.gate_leakage           = mcpat_core->cores[0]->exu->bypass.power.readOp.gate_leakage*m_execution_time;             
   mcpat_core_out.exu.bypass.dynamic                = mcpat_core->cores[0]->exu->bypass.rt_power.readOp.dynamic;
}

//---------------------------------------------------------------------------
// Display Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::displayMcPATCoreEnergy(std::ostream &os)
{
   // Test Output from Data Structure
   string indent2(2, ' ');
   string indent4(4, ' ');
   string indent6(6, ' ');
   string indent8(8, ' ');
   string indent10(10, ' ');
   string indent12(12, ' ');
   string indent14(14, ' ');
   
   // Core
   os << indent2 << "Core:" << std::endl;
   os << indent4 << "Area: "              << mcpat_core_out.core.area << std::endl;               
   os << indent4 << "Static Power: "      << (mcpat_core_out.core.gate_leakage + mcpat_core_out.core.leakage)/m_execution_time << std::endl; 
   os << indent4 << "Dynamic Energy: "    << mcpat_core_out.core.dynamic << std::endl;
   //    Instruction Fetch Unit
   os << indent4 << "Instruction Fetch Unit:" << std::endl;
   os << indent6 << "Area: "              << mcpat_core_out.ifu.ifu.area << std::endl;               
   os << indent6 << "Static Power: "      << (mcpat_core_out.ifu.ifu.gate_leakage + mcpat_core_out.ifu.ifu.leakage)/m_execution_time << std::endl;           
   os << indent6 << "Dynamic Energy: "    << mcpat_core_out.ifu.ifu.dynamic << std::endl;
   //       Instruction Cache
   os << indent8 << "Instruction Cache:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.ifu.icache.area << std::endl;               
   os << indent10 << "Static Power: "      << (mcpat_core_out.ifu.icache.gate_leakage + mcpat_core_out.ifu.icache.leakage)/m_execution_time << std::endl; 
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.ifu.icache.dynamic << std::endl;
   //       Instruction Buffer
   os << indent8 << "Instruction Buffer:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.ifu.IB.area << std::endl;               
   os << indent10 << "Static Power: "      << (mcpat_core_out.ifu.IB.gate_leakage + mcpat_core_out.ifu.IB.leakage)/m_execution_time << std::endl; 
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.ifu.IB.dynamic << std::endl;
   //       Instruction Decoder
   os << indent8 << "Instruction Decoder:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.ifu.ID.area << std::endl;               
   os << indent10 << "Static Power: "      << (mcpat_core_out.ifu.ID.gate_leakage + mcpat_core_out.ifu.ID.leakage)/m_execution_time << std::endl; 
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.ifu.ID.dynamic << std::endl;
   //    Load Store Unit
   os << indent4 << "Load Store Unit:" << std::endl;
   os << indent6 << "Area: "              << mcpat_core_out.lsu.lsu.area << std::endl;               
   os << indent6 << "Static Power: "      << (mcpat_core_out.lsu.lsu.gate_leakage + mcpat_core_out.lsu.lsu.leakage)/m_execution_time << std::endl;             
   os << indent6 << "Dynamic Energy: "    << mcpat_core_out.lsu.lsu.dynamic << std::endl;
   //       Data Cache
   os << indent8 << "Data Cache:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.lsu.dcache.area << std::endl;               
   os << indent10 << "Static Power: "      << (mcpat_core_out.lsu.dcache.gate_leakage + mcpat_core_out.lsu.dcache.leakage)/m_execution_time << std::endl;            
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.lsu.dcache.dynamic << std::endl;
   //       Load/Store Queue
   os << indent8 << "Load/Store Queue:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.lsu.LSQ.area << std::endl;               
   os << indent10 << "Static Power: "      << (mcpat_core_out.lsu.LSQ.gate_leakage + mcpat_core_out.lsu.LSQ.leakage)/m_execution_time << std::endl;            
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.lsu.LSQ.dynamic << std::endl;
   //    Memory Management Unit
   os << indent4 << "Memory Management Unit:" << std::endl;
   os << indent6 << "Area: "              << mcpat_core_out.mmu.mmu.area << std::endl;               
   os << indent6 << "Static Power: "      << (mcpat_core_out.mmu.mmu.gate_leakage + mcpat_core_out.mmu.mmu.leakage)/m_execution_time << std::endl;            
   os << indent6 << "Dynamic Energy: "    << mcpat_core_out.mmu.mmu.dynamic << std::endl;
   //       Itlb
   os << indent8 << "Itlb:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.mmu.itlb.area << std::endl;                          
   os << indent10 << "Static Power: "      << (mcpat_core_out.mmu.itlb.gate_leakage + mcpat_core_out.mmu.itlb.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.mmu.itlb.dynamic << std::endl;
   //       Dtlb
   os << indent8 << "Dtlb:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.mmu.dtlb.area << std::endl;                            
   os << indent10 << "Static Power: "      << (mcpat_core_out.mmu.dtlb.gate_leakage + mcpat_core_out.mmu.dtlb.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.mmu.dtlb.dynamic << std::endl;
   //    Execution Unit
   os << indent4 << "Execution Unit:" << std::endl;
   os << indent6 << "Area: "              << mcpat_core_out.exu.exu.area << std::endl;                         
   os << indent6 << "Static Power: "      << (mcpat_core_out.exu.exu.gate_leakage + mcpat_core_out.exu.exu.leakage)/m_execution_time << std::endl;
   os << indent6 << "Dynamic Energy: "    << mcpat_core_out.exu.exu.dynamic << std::endl;
   //       Register Files
   os << indent8 << "Register Files:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.rfu.rfu.area << std::endl;                          
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.rfu.rfu.gate_leakage + mcpat_core_out.exu.rfu.rfu.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.rfu.rfu.dynamic << std::endl;
   //          Integer RF
   os << indent12 << "Integer RF:" << std::endl;
   os << indent14 << "Area: "              << mcpat_core_out.exu.rfu.IRF.area << std::endl;                            
   os << indent14 << "Static Power: "      << (mcpat_core_out.exu.rfu.IRF.gate_leakage + mcpat_core_out.exu.rfu.IRF.leakage)/m_execution_time << std::endl;
   os << indent14 << "Dynamic Energy: "    << mcpat_core_out.exu.rfu.IRF.dynamic << std::endl;
   //          Floating Point RF
   os << indent12 << "Floating Point RF:" << std::endl;
   os << indent14 << "Area: "              << mcpat_core_out.exu.rfu.FRF.area << std::endl;                          
   os << indent14 << "Static Power: "      << (mcpat_core_out.exu.rfu.FRF.gate_leakage + mcpat_core_out.exu.rfu.FRF.leakage)/m_execution_time << std::endl;
   os << indent14 << "Dynamic Energy: "    << mcpat_core_out.exu.rfu.FRF.dynamic << std::endl;
   //          Register Windows
   os << indent12 << "Register Windows:" << std::endl;
   os << indent14 << "Area: "              << mcpat_core_out.exu.rfu.RFWIN.area << std::endl;                          
   os << indent14 << "Static Power: "      << (mcpat_core_out.exu.rfu.RFWIN.gate_leakage + mcpat_core_out.exu.rfu.RFWIN.leakage)/m_execution_time << std::endl;
   os << indent14 << "Dynamic Energy: "    << mcpat_core_out.exu.rfu.RFWIN.dynamic << std::endl;
   //       Instruction Scheduler
   os << indent8 << "Instruction Scheduler:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.scheu.scheu.area << std::endl;                          
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.scheu.scheu.gate_leakage + mcpat_core_out.exu.scheu.scheu.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.scheu.scheu.dynamic << std::endl;
   //          Instruction Window
   os << indent12 << "Instruction Window:" << std::endl;
   os << indent14 << "Area: "              << mcpat_core_out.exu.scheu.int_inst_window.area << std::endl;                            
   os << indent14 << "Static Power: "      << (mcpat_core_out.exu.scheu.int_inst_window.gate_leakage + mcpat_core_out.exu.scheu.int_inst_window.leakage)/m_execution_time << std::endl;
   os << indent14 << "Dynamic Energy: "    << mcpat_core_out.exu.scheu.int_inst_window.dynamic << std::endl;
   //       Integer ALUs
   os << indent8 << "Integer ALUs:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.exeu.area << std::endl;                       
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.exeu.gate_leakage + mcpat_core_out.exu.exeu.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.exeu.dynamic << std::endl;
   //       Floating Point Units (FPUs)
   os << indent8 << "Floating Point Units (FPUs):" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.fp_u.area << std::endl;                          
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.fp_u.gate_leakage + mcpat_core_out.exu.fp_u.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.fp_u.dynamic << std::endl;
   //       Complex ALUs (Mul/Div)
   os << indent8 << "Complex ALUs (Mul/Div):" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.mul.area << std::endl;                          
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.mul.gate_leakage + mcpat_core_out.exu.mul.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.mul.dynamic << std::endl;
   //       Results Broadcast Bus
   os << indent8 << "Results Broadcast Bus:" << std::endl;
   os << indent10 << "Area: "              << mcpat_core_out.exu.bypass.area << std::endl;                       
   os << indent10 << "Static Power: "      << (mcpat_core_out.exu.bypass.gate_leakage + mcpat_core_out.exu.bypass.leakage)/m_execution_time << std::endl;
   os << indent10 << "Dynamic Energy: "    << mcpat_core_out.exu.bypass.dynamic << std::endl;
}

//---------------------------------------------------------------------------
// Display Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayParam(std::ostream &os)
{
   os << "  Core Parameters:" << std::endl;
   // System Parameters
   // |---- General Parameters
   os << "    clock_rate : " << m_clock_rate << std::endl;
   os << "    core_tech_node : " << m_core_tech_node << std::endl;
   // Architectural Parameters
   // |---- General Parameters
   os << "    instruction_length : " << m_instruction_length << std::endl;
   os << "    opcode_width : " << m_opcode_width << std::endl;
   os << "    machine_type : " << m_machine_type << std::endl;
   os << "    num_hardware_threads : " << m_num_hardware_threads << std::endl;
   os << "    fetch_width : " << m_fetch_width << std::endl;
   os << "    num_instruction_fetch_ports : " << m_num_instruction_fetch_ports << std::endl;
   os << "    decode_width : " << m_decode_width << std::endl;
   os << "    issue_width : " << m_issue_width << std::endl;
   os << "    commit_width : " << m_commit_width << std::endl;
   os << "    fp_issue_width : " << m_fp_issue_width << std::endl;
   os << "    prediction_width : " << m_prediction_width << std::endl;
   os << "    integer_pipeline_depth : " << m_integer_pipeline_depth << std::endl;
   os << "    fp_pipeline_depth : " << m_fp_pipeline_depth << std::endl;
   os << "    ALU_per_core : " << m_ALU_per_core << std::endl;
   os << "    MUL_per_core : " << m_MUL_per_core << std::endl;
   os << "    FPU_per_core : " << m_FPU_per_core << std::endl;
   os << "    instruction_buffer_size : " << m_instruction_buffer_size << std::endl;
   os << "    decoded_stream_buffer_size : " << m_decoded_stream_buffer_size << std::endl;
   // |---- Register File
   os << "    arch_regs_IRF_size : " << m_arch_regs_IRF_size << std::endl;
   os << "    arch_regs_FRF_size : " << m_arch_regs_FRF_size << std::endl;
   os << "    phy_regs_IRF_size : " << m_phy_regs_IRF_size << std::endl;
   os << "    phy_regs_FRF_size : " << m_phy_regs_FRF_size << std::endl;
   // |---- Load-Store Unit
   os << "    LSU_order : " << m_LSU_order << std::endl;
   os << "    store_buffer_size : " << m_store_buffer_size << std::endl;
   os << "    load_buffer_size : " << m_load_buffer_size << std::endl;
   os << "    num_memory_ports : " << m_num_memory_ports << std::endl;
   os << "    RAS_size : " << m_RAS_size << std::endl;
   // |---- OoO Core
   /*os << "    instruction_window_scheme : " << m_instruction_window_scheme << std::endl;
   os << "    instruction_window_size : " << m_instruction_window_size << std::endl;
   os << "    fp_instruction_window_size : " << m_fp_instruction_window_size << std::endl;
   os << "    ROB_size : " << m_ROB_size << std::endl;
   os << "    rename_scheme : " << m_rename_scheme << std::endl;
   // |---- Register Windows (specific to Sun processors)
   os << "    register_windows_size : " << m_register_windows_size << std::endl;*/
}

//---------------------------------------------------------------------------
// Display Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayStats(std::ostream &os)
{
   // Event Counters
   os << "  Core Statistics:" << std::endl;
   os << "    execution_time : " << m_execution_time << std::endl;
   // |-- Used Event Counters
   // |---- Instruction Counters
   
   os << "    total_instructions : " << (UInt64) m_total_instructions << std::endl;
   os << "    int_instructions : " << (UInt64) m_int_instructions << std::endl;
   os << "    fp_instructions : " << (UInt64) m_fp_instructions << std::endl;
   os << "    branch_instructions : " << (UInt64) m_branch_instructions << std::endl;
   os << "    branch_mispredictions : " << (UInt64) m_branch_mispredictions << std::endl;
   os << "    load_instructions : " << (UInt64) m_load_instructions << std::endl;
   os << "    store_instructions : " << (UInt64) m_store_instructions << std::endl;
   os << "    committed_instructions : " << (UInt64) m_committed_instructions << std::endl;
   os << "    committed_int_instructions : " << (UInt64) m_committed_int_instructions << std::endl;
   os << "    committed_fp_instructions : " << (UInt64) m_committed_fp_instructions << std::endl;

   // |---- Cycle Counters
   os << "    total_cycles : " << (UInt64) m_total_cycles << std::endl;
   os << "    idle_cycles : " << (UInt64) m_idle_cycles << std::endl;
   os << "    busy_cycles : " << (UInt64) m_busy_cycles << std::endl;
   
   // |---- Reg File Access Counters
   os << "    int_regfile_reads : " << (UInt64) m_int_regfile_reads << std::endl;
   os << "    int_regfile_writes : " << (UInt64) m_int_regfile_writes << std::endl;
   os << "    fp_regfile_reads : " << (UInt64) m_fp_regfile_reads << std::endl;
   os << "    fp_regfile_writes : " << (UInt64) m_fp_regfile_writes << std::endl;

   // |---- Execution Unit Access Counters
   os << "    ialu_accesses : " << (UInt64) m_ialu_accesses << std::endl;
   os << "    mul_accesses : " << (UInt64) m_mul_accesses << std::endl;
   os << "    fpu_accesses : " << (UInt64) m_fpu_accesses << std::endl;
   os << "    cdb_alu_accesses : " << (UInt64) m_cdb_alu_accesses << std::endl;
   os << "    cdb_mul_accesses : " << (UInt64) m_cdb_mul_accesses << std::endl;
   os << "    cdb_fpu_accesses : " << (UInt64) m_cdb_fpu_accesses << std::endl;

   /*// |-- Unused Event Counters
   // |---- OoO Core Event Counters
   os << "    inst_window_reads : " << m_inst_window_reads << std::endl;
   os << "    inst_window_writes : " << m_inst_window_writes << std::endl;
   os << "    inst_window_wakeup_accesses : " << m_inst_window_wakeup_accesses << std::endl;
   os << "    fp_inst_window_reads : " << m_fp_inst_window_reads << std::endl;
   os << "    fp_inst_window_writes : " << m_fp_inst_window_writes << std::endl;
   os << "    fp_inst_window_wakeup_accesses : " << m_fp_inst_window_wakeup_accesses << std::endl;
   os << "    ROB_reads : " << m_ROB_reads << std::endl;
   os << "    ROB_writes : " << m_ROB_writes << std::endl;
   os << "    rename_accesses : " << m_rename_accesses << std::endl;
   os << "    fp_rename_accesses : " << m_fp_rename_accesses << std::endl;
   // |---- Function Calls and Context Switches
   os << "    function_calls : " << m_function_calls << std::endl;
   os << "    context_switches : " << m_context_switches << std::endl;*/
}

//---------------------------------------------------------------------------
// Fill ParseXML Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::fillCoreParamsIntoXML()
{
   // SYSTEM PARAM
   mcpat_parsexml->sys.number_of_cores = 1;
   mcpat_parsexml->sys.number_of_L1Directories = 0;
   mcpat_parsexml->sys.number_of_L2Directories = 0;
   mcpat_parsexml->sys.number_of_L2s = 0;
   mcpat_parsexml->sys.number_of_L3s = 0;
   mcpat_parsexml->sys.number_of_NoCs = 0;
   mcpat_parsexml->sys.mc.number_mcs = 0;
   mcpat_parsexml->sys.homogeneous_cores = 1;
   mcpat_parsexml->sys.core_tech_node = m_core_tech_node;
   int i;
   for (i = 0; i <= 63; i++)
   {
      mcpat_parsexml->sys.core[i].clock_rate = m_clock_rate;
      // SYSTEM.CORE PARAM
      // |---- General Parameters
      //cout << "|---- General Parameters" << endl;
      mcpat_parsexml->sys.core[i].instruction_length = m_instruction_length;
      mcpat_parsexml->sys.core[i].opcode_width = m_opcode_width;
      mcpat_parsexml->sys.core[i].machine_type = m_machine_type;
      mcpat_parsexml->sys.core[i].number_hardware_threads = m_num_hardware_threads;
      mcpat_parsexml->sys.core[i].fetch_width = m_fetch_width;
      mcpat_parsexml->sys.core[i].number_instruction_fetch_ports = m_num_instruction_fetch_ports;
      mcpat_parsexml->sys.core[i].decode_width = m_decode_width;
      mcpat_parsexml->sys.core[i].issue_width = m_issue_width;
      mcpat_parsexml->sys.core[i].commit_width = m_commit_width;
      mcpat_parsexml->sys.core[i].fp_issue_width = m_fp_issue_width;
      mcpat_parsexml->sys.core[i].predictor.prediction_width = m_prediction_width;
      mcpat_parsexml->sys.core[i].pipeline_depth[0] = m_integer_pipeline_depth;
      mcpat_parsexml->sys.core[i].pipeline_depth[1] = m_fp_pipeline_depth;
      mcpat_parsexml->sys.core[i].ALU_per_core = m_ALU_per_core;
      mcpat_parsexml->sys.core[i].MUL_per_core = m_MUL_per_core;
      mcpat_parsexml->sys.core[i].FPU_per_core = m_FPU_per_core;
      mcpat_parsexml->sys.core[i].instruction_buffer_size = m_instruction_buffer_size;
      mcpat_parsexml->sys.core[i].decoded_stream_buffer_size = m_decoded_stream_buffer_size;
      // |---- Register File
      //cout << "|---- Register File" << endl;
      mcpat_parsexml->sys.core[i].archi_Regs_IRF_size = m_arch_regs_IRF_size;
      mcpat_parsexml->sys.core[i].archi_Regs_FRF_size = m_arch_regs_FRF_size;
      mcpat_parsexml->sys.core[i].phy_Regs_IRF_size = m_phy_regs_IRF_size;
      mcpat_parsexml->sys.core[i].phy_Regs_FRF_size = m_phy_regs_FRF_size;
      // |---- Load-Store Unit
      //cout << "|---- Load-Store Unit" << endl;
      strcpy(mcpat_parsexml->sys.core[i].LSU_order,m_LSU_order.c_str());
      mcpat_parsexml->sys.core[i].store_buffer_size = m_store_buffer_size;
      mcpat_parsexml->sys.core[i].load_buffer_size = m_load_buffer_size;
      mcpat_parsexml->sys.core[i].memory_ports = m_num_memory_ports;
      mcpat_parsexml->sys.core[i].RAS_size = m_RAS_size;      
      // |---- OoO Core
      //cout << "|---- OoO Core" << endl;
      mcpat_parsexml->sys.core[i].instruction_window_scheme = m_instruction_window_scheme;
      mcpat_parsexml->sys.core[i].instruction_window_size = m_instruction_window_size;
      mcpat_parsexml->sys.core[i].fp_instruction_window_size = m_fp_instruction_window_size;
      mcpat_parsexml->sys.core[i].ROB_size = m_ROB_size;
      mcpat_parsexml->sys.core[i].rename_scheme = m_rename_scheme;
      // |---- Register Windows (specific to Sun processors)
      //cout << "|---- Register Windows" << endl;
      mcpat_parsexml->sys.core[i].register_windows_size = m_register_windows_size;
   }
}

//---------------------------------------------------------------------------
// Fill ParseXML Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::fillCoreStatsIntoXML()
{
   // SYSTEM STATS
   mcpat_parsexml->sys.total_cycles = m_total_cycles;
   int i;
   for (i=0; i<=63; i++)
   {  
      // SYSTEM.CORE STATS
      // |-- Used Event Counters
      // |---- Instruction Counters
      //cout << "|---- Instruction Counters" << endl;
      mcpat_parsexml->sys.core[i].total_instructions = m_total_instructions;
      mcpat_parsexml->sys.core[i].int_instructions = m_int_instructions;
      mcpat_parsexml->sys.core[i].fp_instructions = m_fp_instructions;
      mcpat_parsexml->sys.core[i].branch_instructions = m_branch_instructions;
      mcpat_parsexml->sys.core[i].branch_mispredictions = m_branch_mispredictions;
      mcpat_parsexml->sys.core[i].load_instructions = m_load_instructions;
      mcpat_parsexml->sys.core[i].store_instructions = m_store_instructions;
      mcpat_parsexml->sys.core[i].committed_instructions = m_committed_instructions;
      mcpat_parsexml->sys.core[i].committed_instructions = m_committed_int_instructions;
      mcpat_parsexml->sys.core[i].committed_instructions = m_committed_fp_instructions;
      // |---- Cycle Counters
      //cout << "|---- Cycle Counters" << endl;
      mcpat_parsexml->sys.core[i].total_cycles = m_total_cycles;
      mcpat_parsexml->sys.core[i].idle_cycles = m_idle_cycles;
      mcpat_parsexml->sys.core[i].busy_cycles = m_busy_cycles;
      // |---- Reg File Access Counters
      //cout << "|---- Reg File Access Counters" << endl;
      mcpat_parsexml->sys.core[i].int_regfile_reads = m_int_regfile_reads;
      mcpat_parsexml->sys.core[i].int_regfile_writes = m_int_regfile_writes;
      mcpat_parsexml->sys.core[i].float_regfile_reads = m_fp_regfile_reads;
      mcpat_parsexml->sys.core[i].float_regfile_writes = m_fp_regfile_writes;
      // |---- Execution Unit Access Counters
      //cout << "|---- Execution Unit Access Counters" << endl;
      mcpat_parsexml->sys.core[i].ialu_accesses = m_ialu_accesses;
      mcpat_parsexml->sys.core[i].mul_accesses = m_mul_accesses;
      mcpat_parsexml->sys.core[i].fpu_accesses = m_fpu_accesses;
      mcpat_parsexml->sys.core[i].cdb_alu_accesses = m_cdb_alu_accesses;
      mcpat_parsexml->sys.core[i].cdb_mul_accesses = m_cdb_mul_accesses;
      mcpat_parsexml->sys.core[i].cdb_fpu_accesses = m_cdb_fpu_accesses;
      // |-- Unused Event Counters
      // |---- OoO Core Event Counters
      //cout << "|---- OoO Core Event Counters" << endl;
      mcpat_parsexml->sys.core[i].inst_window_reads = m_inst_window_reads;
      mcpat_parsexml->sys.core[i].inst_window_writes = m_inst_window_writes;
      mcpat_parsexml->sys.core[i].inst_window_wakeup_accesses = m_inst_window_wakeup_accesses;
      mcpat_parsexml->sys.core[i].fp_inst_window_reads = m_fp_inst_window_reads;
      mcpat_parsexml->sys.core[i].fp_inst_window_writes = m_fp_inst_window_writes;
      mcpat_parsexml->sys.core[i].fp_inst_window_wakeup_accesses = m_fp_inst_window_wakeup_accesses;
      mcpat_parsexml->sys.core[i].ROB_reads = m_ROB_reads;
      mcpat_parsexml->sys.core[i].ROB_writes = m_ROB_writes;
      mcpat_parsexml->sys.core[i].rename_accesses = m_rename_accesses;
      mcpat_parsexml->sys.core[i].fp_rename_accesses = m_fp_rename_accesses;
      // |---- Function Calls and Context Switches
      //cout << "|---- Function Calls and Context Switches" << endl;
      mcpat_parsexml->sys.core[i].function_calls = m_function_calls;
      mcpat_parsexml->sys.core[i].context_switches = m_context_switches;
   }
}

// Dummy Implementations

__attribute__((weak)) McPATCoreInterface::McPATInstructionType
getMcPATInstructionType(InstructionType type)
{
   return (McPATCoreInterface::INTEGER_INST);
}

__attribute__((weak)) bool
isIntegerReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) bool
isFloatingPointReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) bool
isXMMReg(UInt32 reg_id)
{
   return false;
}

__attribute__((weak)) McPATCoreInterface::ExecutionUnitList
getExecutionUnitAccessList(InstructionType type)
{
   McPATCoreInterface::ExecutionUnitList access_list;
   access_list.push_back(McPATCoreInterface::ALU);
   return access_list;
}
