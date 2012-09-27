/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

// [graphite]

#include <stdio.h>
#include "mcpat_core_interface.h"
#include "XML_Parse.h"
#include "processor.h"
#include <string.h>
#include <iostream>

using namespace std;

namespace McPAT
{

//---------------------------------------------------------------------------
// McPAT Core Interface Constructor
//---------------------------------------------------------------------------
McPATCoreInterface::McPATCoreInterface()
{
   // Initialize Architectural Parameters
   initializeArchitecturalParameters();
   // Initialize Event Counters
   initializeEventCounters();
   // Initialize Output Data Structure
   initializeOutputDataStructure();

   // Make a ParseXML Object and Initialize it
   mcpat_parsexml = new ParseXML();

   // Initialize ParseXML Params and Stats
   mcpat_parsexml->initialize();
   mcpat_parsexml->setNiagara1();

   // Fill the ParseXML's Core Params from McPATCoreInterface
   mcpat_parsexml->fillCoreParamsFromMcPATCoreInterface(this);

   // Make a Processor Object from the ParseXML
   mcpat_processor = new Processor(mcpat_parsexml);
}

//---------------------------------------------------------------------------
// McPAT Core Interface Destructor
//---------------------------------------------------------------------------
McPATCoreInterface::~McPATCoreInterface()
{
   delete mcpat_parsexml;
   delete mcpat_processor;
}

//---------------------------------------------------------------------------
// Initialize Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::initializeArchitecturalParameters()
{
   // System Parameters
   // |---- General Parameters
   m_clock_rate = 1000;
   m_core_tech_node = 45;
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
   m_load_buffer_size = 32;
   m_store_buffer_size = 32;
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
   mcpat_             core_out.exu.scheu.int_inst_window.area        = 0;
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

//---------------------------------------------------------------------------
// Compute Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::computeMcPATCoreEnergy()
{
   // Fill the ParseXML's Core Event Stats from McPATCoreInterface
   mcpat_parsexml->fillCoreStatsFromMcPATCoreInterface(this);

   // Compute Energy from Processor
   mcpat_processor->computeEnergy();

   // Execution Time
   executionTime = (mcpat_processor->cores[0]->coredynp.executionTime);
   //cout << "executionTime = " << executionTime << endl;

   // Store Energy into Data Structure
   // Core
   mcpat_core_out.core.area                   = mcpat_processor->cores[0]->area.get_area()*1e-6;                     
   mcpat_core_out.core.leakage                = mcpat_processor->cores[0]->power.readOp.leakage*executionTime;                  
   mcpat_core_out.core.longer_channel_leakage = mcpat_processor->cores[0]->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.core.gate_leakage           = mcpat_processor->cores[0]->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.core.dynamic                = mcpat_processor->cores[0]->rt_power.readOp.dynamic;
   //    Instruction Fetch Unit
   mcpat_core_out.ifu.ifu.area                   = mcpat_processor->cores[0]->ifu->area.get_area()*1e-6;                     
   mcpat_core_out.ifu.ifu.leakage                = mcpat_processor->cores[0]->ifu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.ifu.ifu.longer_channel_leakage = mcpat_processor->cores[0]->ifu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.ifu.ifu.gate_leakage           = mcpat_processor->cores[0]->ifu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.ifu.ifu.dynamic                = mcpat_processor->cores[0]->ifu->rt_power.readOp.dynamic;
   //       Instruction Cache
   mcpat_core_out.ifu.icache.area                   = mcpat_processor->cores[0]->ifu->icache.area.get_area()*1e-6;                     
   mcpat_core_out.ifu.icache.leakage                = mcpat_processor->cores[0]->ifu->icache.power.readOp.leakage*executionTime;                  
   mcpat_core_out.ifu.icache.longer_channel_leakage = mcpat_processor->cores[0]->ifu->icache.power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.ifu.icache.gate_leakage           = mcpat_processor->cores[0]->ifu->icache.power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.ifu.icache.dynamic                = mcpat_processor->cores[0]->ifu->icache.rt_power.readOp.dynamic;
   //       Instruction Buffer
   mcpat_core_out.ifu.IB.area                       = mcpat_processor->cores[0]->ifu->IB->area.get_area()*1e-6;                     
   mcpat_core_out.ifu.IB.leakage                    = mcpat_processor->cores[0]->ifu->IB->power.readOp.leakage*executionTime;                  
   mcpat_core_out.ifu.IB.longer_channel_leakage     = mcpat_processor->cores[0]->ifu->IB->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.ifu.IB.gate_leakage               = mcpat_processor->cores[0]->ifu->IB->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.ifu.IB.dynamic                    = mcpat_processor->cores[0]->ifu->IB->rt_power.readOp.dynamic;
   //       Instruction Decoder
   mcpat_core_out.ifu.ID.area                       = (mcpat_processor->cores[0]->ifu->ID_inst->area.get_area() +
         mcpat_processor->cores[0]->ifu->ID_operand->area.get_area() +
         mcpat_processor->cores[0]->ifu->ID_misc->area.get_area())*
         mcpat_processor->cores[0]->ifu->coredynp.decodeW*1e-6;                     
   mcpat_core_out.ifu.ID.leakage                    = (mcpat_processor->cores[0]->ifu->ID_inst->power.readOp.leakage +
         mcpat_processor->cores[0]->ifu->ID_operand->power.readOp.leakage +
         mcpat_processor->cores[0]->ifu->ID_misc->power.readOp.leakage);                
   mcpat_core_out.ifu.ID.longer_channel_leakage     = (mcpat_processor->cores[0]->ifu->ID_inst->power.readOp.longer_channel_leakage +
         mcpat_processor->cores[0]->ifu->ID_operand->power.readOp.longer_channel_leakage +
         mcpat_processor->cores[0]->ifu->ID_misc->power.readOp.longer_channel_leakage);   
   mcpat_core_out.ifu.ID.gate_leakage               = (mcpat_processor->cores[0]->ifu->ID_inst->power.readOp.gate_leakage +
         mcpat_processor->cores[0]->ifu->ID_operand->power.readOp.gate_leakage +
         mcpat_processor->cores[0]->ifu->ID_misc->power.readOp.gate_leakage);          
   mcpat_core_out.ifu.ID.dynamic                    = (mcpat_processor->cores[0]->ifu->ID_inst->rt_power.readOp.dynamic +
         mcpat_processor->cores[0]->ifu->ID_operand->rt_power.readOp.dynamic +
         mcpat_processor->cores[0]->ifu->ID_misc->rt_power.readOp.dynamic);
   //    Load Store Unit
   mcpat_core_out.lsu.lsu.area                   = mcpat_processor->cores[0]->lsu->area.get_area()*1e-6;                     
   mcpat_core_out.lsu.lsu.leakage                = mcpat_processor->cores[0]->lsu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.lsu.lsu.longer_channel_leakage = mcpat_processor->cores[0]->lsu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.lsu.lsu.gate_leakage           = mcpat_processor->cores[0]->lsu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.lsu.lsu.dynamic                = mcpat_processor->cores[0]->lsu->rt_power.readOp.dynamic;
   //       Data Cache
   mcpat_core_out.lsu.dcache.area                   = mcpat_processor->cores[0]->lsu->dcache.area.get_area()*1e-6;                     
   mcpat_core_out.lsu.dcache.leakage                = mcpat_processor->cores[0]->lsu->dcache.power.readOp.leakage*executionTime;                  
   mcpat_core_out.lsu.dcache.longer_channel_leakage = mcpat_processor->cores[0]->lsu->dcache.power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.lsu.dcache.gate_leakage           = mcpat_processor->cores[0]->lsu->dcache.power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.lsu.dcache.dynamic                = mcpat_processor->cores[0]->lsu->dcache.rt_power.readOp.dynamic;
   //       Load/Store Queue
   mcpat_core_out.lsu.LSQ.area                      = mcpat_processor->cores[0]->lsu->LSQ->area.get_area()*1e-6;                     
   mcpat_core_out.lsu.LSQ.leakage                   = mcpat_processor->cores[0]->lsu->LSQ->power.readOp.leakage*executionTime;                  
   mcpat_core_out.lsu.LSQ.longer_channel_leakage    = mcpat_processor->cores[0]->lsu->LSQ->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.lsu.LSQ.gate_leakage              = mcpat_processor->cores[0]->lsu->LSQ->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.lsu.LSQ.dynamic                   = mcpat_processor->cores[0]->lsu->LSQ->rt_power.readOp.dynamic;
   //    Memory Management Unit
   mcpat_core_out.mmu.mmu.area                   = mcpat_processor->cores[0]->mmu->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.mmu.leakage                = mcpat_processor->cores[0]->mmu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.mmu.mmu.longer_channel_leakage = mcpat_processor->cores[0]->mmu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.mmu.mmu.gate_leakage           = mcpat_processor->cores[0]->mmu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.mmu.mmu.dynamic                = mcpat_processor->cores[0]->mmu->rt_power.readOp.dynamic;
   //       Itlb
   mcpat_core_out.mmu.itlb.area                     = mcpat_processor->cores[0]->mmu->itlb->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.itlb.leakage                  = mcpat_processor->cores[0]->mmu->itlb->power.readOp.leakage*executionTime;                  
   mcpat_core_out.mmu.itlb.longer_channel_leakage   = mcpat_processor->cores[0]->mmu->itlb->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.mmu.itlb.gate_leakage             = mcpat_processor->cores[0]->mmu->itlb->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.mmu.itlb.dynamic                  = mcpat_processor->cores[0]->mmu->itlb->rt_power.readOp.dynamic;
   //       Dtlb
   mcpat_core_out.mmu.dtlb.area                     = mcpat_processor->cores[0]->mmu->dtlb->area.get_area()*1e-6;                     
   mcpat_core_out.mmu.dtlb.leakage                  = mcpat_processor->cores[0]->mmu->dtlb->power.readOp.leakage*executionTime;                  
   mcpat_core_out.mmu.dtlb.longer_channel_leakage   = mcpat_processor->cores[0]->mmu->dtlb->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.mmu.dtlb.gate_leakage             = mcpat_processor->cores[0]->mmu->dtlb->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.mmu.dtlb.dynamic                  = mcpat_processor->cores[0]->mmu->dtlb->rt_power.readOp.dynamic;
   //    Execution Unit
   mcpat_core_out.exu.exu.area                   = mcpat_processor->cores[0]->exu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.exu.leakage                = mcpat_processor->cores[0]->exu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.exu.longer_channel_leakage = mcpat_processor->cores[0]->exu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.exu.gate_leakage           = mcpat_processor->cores[0]->exu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.exu.dynamic                = mcpat_processor->cores[0]->exu->rt_power.readOp.dynamic;
   //       Register Files
   mcpat_core_out.exu.rfu.rfu.area                        = mcpat_processor->cores[0]->exu->rfu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.rfu.leakage                     = mcpat_processor->cores[0]->exu->rfu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.rfu.rfu.longer_channel_leakage      = mcpat_processor->cores[0]->exu->rfu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.rfu.rfu.gate_leakage                = mcpat_processor->cores[0]->exu->rfu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.rfu.rfu.dynamic                     = mcpat_processor->cores[0]->exu->rfu->rt_power.readOp.dynamic;
   //          Integer RF
   mcpat_core_out.exu.rfu.IRF.area                           = mcpat_processor->cores[0]->exu->rfu->IRF->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.IRF.leakage                        = mcpat_processor->cores[0]->exu->rfu->IRF->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.rfu.IRF.longer_channel_leakage         = mcpat_processor->cores[0]->exu->rfu->IRF->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.rfu.IRF.gate_leakage                   = mcpat_processor->cores[0]->exu->rfu->IRF->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.rfu.IRF.dynamic                        = mcpat_processor->cores[0]->exu->rfu->IRF->rt_power.readOp.dynamic;
   //          Floating Point RF
   mcpat_core_out.exu.rfu.FRF.area                           = mcpat_processor->cores[0]->exu->rfu->FRF->area.get_area()*1e-6;                     
   mcpat_core_out.exu.rfu.FRF.leakage                        = mcpat_processor->cores[0]->exu->rfu->FRF->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.rfu.FRF.longer_channel_leakage         = mcpat_processor->cores[0]->exu->rfu->FRF->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.rfu.FRF.gate_leakage                   = mcpat_processor->cores[0]->exu->rfu->FRF->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.rfu.FRF.dynamic                        = mcpat_processor->cores[0]->exu->rfu->FRF->rt_power.readOp.dynamic;
   //          Register Windows
   if (mcpat_processor->cores[0]->exu->rfu->RFWIN)
   {
      mcpat_core_out.exu.rfu.RFWIN.area                         = mcpat_processor->cores[0]->exu->rfu->RFWIN->area.get_area()*1e-6;                     
      mcpat_core_out.exu.rfu.RFWIN.leakage                      = mcpat_processor->cores[0]->exu->rfu->RFWIN->power.readOp.leakage*executionTime;                  
      mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage       = mcpat_processor->cores[0]->exu->rfu->RFWIN->power.readOp.longer_channel_leakage*executionTime;   
      mcpat_core_out.exu.rfu.RFWIN.gate_leakage                 = mcpat_processor->cores[0]->exu->rfu->RFWIN->power.readOp.gate_leakage*executionTime;             
      mcpat_core_out.exu.rfu.RFWIN.dynamic                      = mcpat_processor->cores[0]->exu->rfu->RFWIN->rt_power.readOp.dynamic;
   }
   //       Instruction Scheduler
   mcpat_core_out.exu.scheu.scheu.area                    = mcpat_processor->cores[0]->exu->scheu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.scheu.scheu.leakage                 = mcpat_processor->cores[0]->exu->scheu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.scheu.scheu.longer_channel_leakage  = mcpat_processor->cores[0]->exu->scheu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.scheu.scheu.gate_leakage            = mcpat_processor->cores[0]->exu->scheu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.scheu.scheu.dynamic                 = mcpat_processor->cores[0]->exu->scheu->rt_power.readOp.dynamic;
   //          Instruction Window
   if (mcpat_processor->cores[0]->exu->scheu->int_inst_window)
   {
      mcpat_core_out.exu.scheu.int_inst_window.area                    = mcpat_processor->cores[0]->exu->scheu->int_inst_window->area.get_area()*1e-6;                     
      mcpat_core_out.exu.scheu.int_inst_window.leakage                 = mcpat_processor->cores[0]->exu->scheu->int_inst_window->power.readOp.leakage*executionTime;                  
      mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage  = mcpat_processor->cores[0]->exu->scheu->int_inst_window->power.readOp.longer_channel_leakage*executionTime;   
      mcpat_core_out.exu.scheu.int_inst_window.gate_leakage            = mcpat_processor->cores[0]->exu->scheu->int_inst_window->power.readOp.gate_leakage*executionTime;             
      mcpat_core_out.exu.scheu.int_inst_window.dynamic                 = mcpat_processor->cores[0]->exu->scheu->int_inst_window->rt_power.readOp.dynamic;
   }
   //       Integer ALUs
   mcpat_core_out.exu.exeu.area                     = mcpat_processor->cores[0]->exu->exeu->area.get_area()*1e-6;                     
   mcpat_core_out.exu.exeu.leakage                  = mcpat_processor->cores[0]->exu->exeu->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.exeu.longer_channel_leakage   = mcpat_processor->cores[0]->exu->exeu->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.exeu.gate_leakage             = mcpat_processor->cores[0]->exu->exeu->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.exeu.dynamic                  = mcpat_processor->cores[0]->exu->exeu->rt_power.readOp.dynamic;
   //       Floating Point Units (FPUs)
   mcpat_core_out.exu.fp_u.area                     = mcpat_processor->cores[0]->exu->fp_u->area.get_area()*1e-6;                     
   mcpat_core_out.exu.fp_u.leakage                  = mcpat_processor->cores[0]->exu->fp_u->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.fp_u.longer_channel_leakage   = mcpat_processor->cores[0]->exu->fp_u->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.fp_u.gate_leakage             = mcpat_processor->cores[0]->exu->fp_u->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.fp_u.dynamic                  = mcpat_processor->cores[0]->exu->fp_u->rt_power.readOp.dynamic;
   //       Complex ALUs (Mul/Div)
   mcpat_core_out.exu.mul.area                      = mcpat_processor->cores[0]->exu->mul->area.get_area()*1e-6;                     
   mcpat_core_out.exu.mul.leakage                   = mcpat_processor->cores[0]->exu->mul->power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.mul.longer_channel_leakage    = mcpat_processor->cores[0]->exu->mul->power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.mul.gate_leakage              = mcpat_processor->cores[0]->exu->mul->power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.mul.dynamic                   = mcpat_processor->cores[0]->exu->mul->rt_power.readOp.dynamic;
   //       Results Broadcast Bus
   mcpat_core_out.exu.bypass.area                   = mcpat_processor->cores[0]->exu->bypass.area.get_area()*1e-6;                     
   mcpat_core_out.exu.bypass.leakage                = mcpat_processor->cores[0]->exu->bypass.power.readOp.leakage*executionTime;                  
   mcpat_core_out.exu.bypass.longer_channel_leakage = mcpat_processor->cores[0]->exu->bypass.power.readOp.longer_channel_leakage*executionTime;   
   mcpat_core_out.exu.bypass.gate_leakage           = mcpat_processor->cores[0]->exu->bypass.power.readOp.gate_leakage*executionTime;             
   mcpat_core_out.exu.bypass.dynamic                = mcpat_processor->cores[0]->exu->bypass.rt_power.readOp.dynamic;
}

//---------------------------------------------------------------------------
// Display Energy from McPat
//---------------------------------------------------------------------------
void McPATCoreInterface::displayMcPATCoreEnergy()
{
   // Test Output from Data Structure
   int indent = 2;
   string indent_str(indent, ' ');
   string indent_str_next(indent+2, ' ');
   string next_indent_str(indent+4, ' ');
   string next_indent_str_next(indent+4+2, ' ');
   string next_next_indent_str(indent+4+4, ' ');
   string next_next_indent_str_next(indent+4+4+2, ' ');
   cout <<"*****************************************************************************************"<<endl;
   cout << endl;
   // Core
   cout << "Core:" << endl;
   cout << indent_str << "Area = "                   << mcpat_core_out.core.area                   << " mm^2" << endl;               
   cout << indent_str << "Leakage = "                << mcpat_core_out.core.leakage                << " W" << endl;             
   cout << indent_str << "Longer Channel Leakage = " << mcpat_core_out.core.longer_channel_leakage << " W" << endl;   
   cout << indent_str << "Gate Leakage = "           << mcpat_core_out.core.gate_leakage           << " W" << endl;             
   cout << indent_str << "Runtime Dynamic = "        << mcpat_core_out.core.dynamic                << " J" << endl;
   cout << endl;
   //    Instruction Fetch Unit
   cout << indent_str << "Instruction Fetch Unit:" << endl;
   cout << indent_str_next << "Area = "                   << mcpat_core_out.ifu.ifu.area                   << " mm^2" << endl;               
   cout << indent_str_next << "Leakage = "                << mcpat_core_out.ifu.ifu.leakage                << " W" << endl;             
   cout << indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.ifu.ifu.longer_channel_leakage << " W" << endl;   
   cout << indent_str_next << "Gate Leakage = "           << mcpat_core_out.ifu.ifu.gate_leakage           << " W" << endl;             
   cout << indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.ifu.ifu.dynamic                << " J" << endl;
   cout << endl;
   //       Instruction Cache
   cout << next_indent_str << "Instruction Cache:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.ifu.icache.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.ifu.icache.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.ifu.icache.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.ifu.icache.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.ifu.icache.dynamic                << " J" << endl;
   cout << endl;
   //       Instruction Buffer
   cout << next_indent_str << "Instruction Buffer:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.ifu.IB.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.ifu.IB.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.ifu.IB.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.ifu.IB.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.ifu.IB.dynamic                << " J" << endl;
   cout << endl;
   //       Instruction Decoder
   cout << next_indent_str << "Instruction Decoder:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.ifu.ID.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.ifu.ID.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.ifu.ID.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.ifu.ID.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.ifu.ID.dynamic                << " J" << endl;
   cout << endl;
   //    Load Store Unit
   cout << indent_str << "Load Store Unit:" << endl;
   cout << indent_str_next << "Area = "                   << mcpat_core_out.lsu.lsu.area                   << " mm^2" << endl;               
   cout << indent_str_next << "Leakage = "                << mcpat_core_out.lsu.lsu.leakage                << " W" << endl;             
   cout << indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.lsu.lsu.longer_channel_leakage << " W" << endl;   
   cout << indent_str_next << "Gate Leakage = "           << mcpat_core_out.lsu.lsu.gate_leakage           << " W" << endl;             
   cout << indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.lsu.lsu.dynamic                << " J" << endl;
   cout << endl;
   //       Data Cache
   cout << next_indent_str << "Instruction Cache:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.lsu.dcache.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.lsu.dcache.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.lsu.dcache.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.lsu.dcache.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.lsu.dcache.dynamic                << " J" << endl;
   cout << endl;
   //       Load/Store Queue
   cout << next_indent_str << "Load/Store Queue:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.lsu.LSQ.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.lsu.LSQ.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.lsu.LSQ.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.lsu.LSQ.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.lsu.LSQ.dynamic                << " J" << endl;
   cout << endl;
   //    Memory Management Unit
   cout << indent_str << "Memory Management Unit:" << endl;
   cout << indent_str_next << "Area = "                   << mcpat_core_out.mmu.mmu.area                   << " mm^2" << endl;               
   cout << indent_str_next << "Leakage = "                << mcpat_core_out.mmu.mmu.leakage                << " W" << endl;             
   cout << indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.mmu.mmu.longer_channel_leakage << " W" << endl;   
   cout << indent_str_next << "Gate Leakage = "           << mcpat_core_out.mmu.mmu.gate_leakage           << " W" << endl;             
   cout << indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.mmu.mmu.dynamic                << " J" << endl;
   cout << endl;
   //       Itlb
   cout << next_indent_str << "Itlb:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.mmu.itlb.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.mmu.itlb.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.mmu.itlb.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.mmu.itlb.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.mmu.itlb.dynamic                << " J" << endl;
   cout << endl;
   //       Dtlb
   cout << next_indent_str << "Dtlb:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.mmu.dtlb.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.mmu.dtlb.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.mmu.dtlb.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.mmu.dtlb.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.mmu.dtlb.dynamic                << " J" << endl;
   cout << endl;
   //    Execution Unit
   cout << indent_str << "Execution Unit:" << endl;
   cout << indent_str_next << "Area = "                   << mcpat_core_out.exu.exu.area                   << " mm^2" << endl;               
   cout << indent_str_next << "Leakage = "                << mcpat_core_out.exu.exu.leakage                << " W" << endl;             
   cout << indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.exu.longer_channel_leakage << " W" << endl;   
   cout << indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.exu.gate_leakage           << " W" << endl;             
   cout << indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.exu.dynamic                << " J" << endl;
   cout << endl;
   //       Register Files
   cout << next_indent_str << "Register Files:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.rfu.rfu.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.rfu.rfu.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.rfu.rfu.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.rfu.rfu.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.rfu.rfu.dynamic                << " J" << endl;
   cout << endl;
   //          Integer RF
   cout << next_next_indent_str << "Integer RF:" << endl;
   cout << next_next_indent_str_next << "Area = "                   << mcpat_core_out.exu.rfu.IRF.area                   << " mm^2" << endl;               
   cout << next_next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.rfu.IRF.leakage                << " W" << endl;             
   cout << next_next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.rfu.IRF.longer_channel_leakage << " W" << endl;   
   cout << next_next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.rfu.IRF.gate_leakage           << " W" << endl;             
   cout << next_next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.rfu.IRF.dynamic                << " J" << endl;
   cout << endl;
   //          Floating Point RF
   cout << next_next_indent_str << "Floating Point RF:" << endl;
   cout << next_next_indent_str_next << "Area = "                   << mcpat_core_out.exu.rfu.FRF.area                   << " mm^2" << endl;               
   cout << next_next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.rfu.FRF.leakage                << " W" << endl;             
   cout << next_next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.rfu.FRF.longer_channel_leakage << " W" << endl;   
   cout << next_next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.rfu.FRF.gate_leakage           << " W" << endl;             
   cout << next_next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.rfu.FRF.dynamic                << " J" << endl;
   cout << endl;
   //          Register Windows
   cout << next_next_indent_str << "Register Windows:" << endl;
   cout << next_next_indent_str_next << "Area = "                   << mcpat_core_out.exu.rfu.RFWIN.area                   << " mm^2" << endl;               
   cout << next_next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.rfu.RFWIN.leakage                << " W" << endl;             
   cout << next_next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.rfu.RFWIN.longer_channel_leakage << " W" << endl;   
   cout << next_next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.rfu.RFWIN.gate_leakage           << " W" << endl;             
   cout << next_next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.rfu.RFWIN.dynamic                << " J" << endl;
   cout << endl;
   //       Instruction Scheduler
   cout << next_indent_str << "Instruction Scheduler:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.scheu.scheu.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.scheu.scheu.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.scheu.scheu.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.scheu.scheu.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.scheu.scheu.dynamic                << " J" << endl;
   cout << endl;
   //          Instruction Window
   cout << next_next_indent_str << "Instruction Window:" << endl;
   cout << next_next_indent_str_next << "Area = "                   << mcpat_core_out.exu.scheu.int_inst_window.area                   << " mm^2" << endl;               
   cout << next_next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.scheu.int_inst_window.leakage                << " W" << endl;             
   cout << next_next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.scheu.int_inst_window.longer_channel_leakage << " W" << endl;   
   cout << next_next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.scheu.int_inst_window.gate_leakage           << " W" << endl;             
   cout << next_next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.scheu.int_inst_window.dynamic                << " J" << endl;
   cout << endl;
   //       Integer ALUs
   cout << next_indent_str << "Integer ALUs:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.exeu.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.exeu.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.exeu.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.exeu.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.exeu.dynamic                << " J" << endl;
   cout << endl;
   //       Floating Point Units (FPUs)
   cout << next_indent_str << "Floating Point Units (FPUs):" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.fp_u.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.fp_u.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.fp_u.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.fp_u.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.fp_u.dynamic                << " J" << endl;
   cout << endl;
   //       Complex ALUs (Mul/Div)
   cout << next_indent_str << "Complex ALUs (Mul/Div):" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.mul.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.mul.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.mul.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.mul.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.mul.dynamic                << " J" << endl;
   cout << endl;
   //       Results Broadcast Bus
   cout << next_indent_str << "Results Broadcast Bus:" << endl;
   cout << next_indent_str_next << "Area = "                   << mcpat_core_out.exu.bypass.area                   << " mm^2" << endl;               
   cout << next_indent_str_next << "Leakage = "                << mcpat_core_out.exu.bypass.leakage                << " W" << endl;             
   cout << next_indent_str_next << "Longer Channel Leakage = " << mcpat_core_out.exu.bypass.longer_channel_leakage << " W" << endl;   
   cout << next_indent_str_next << "Gate Leakage = "           << mcpat_core_out.exu.bypass.gate_leakage           << " W" << endl;             
   cout << next_indent_str_next << "Runtime Dynamic = "        << mcpat_core_out.exu.bypass.dynamic                << " J" << endl;
   cout << endl;
   cout <<"*****************************************************************************************"<<endl;
}

//---------------------------------------------------------------------------
// Display Architectural Parameters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayParam()
{
   cout << "  Core Parameters:" << std::endl;
   // System Parameters
   // |---- General Parameters
   cout << "    clock_rate : " << m_clock_rate << std::endl;
   cout << "    core_tech_node : " << m_core_tech_node << std::endl;
   // Architectural Parameters
   // |---- General Parameters
   cout << "    instruction_length : " << m_instruction_length << std::endl;
   cout << "    opcode_width : " << m_opcode_width << std::endl;
   cout << "    machine_type : " << m_machine_type << std::endl;
   cout << "    num_hardware_threads : " << m_num_hardware_threads << std::endl;
   cout << "    fetch_width : " << m_fetch_width << std::endl;
   cout << "    num_instruction_fetch_ports : " << m_num_instruction_fetch_ports << std::endl;
   cout << "    decode_width : " << m_decode_width << std::endl;
   cout << "    issue_width : " << m_issue_width << std::endl;
   cout << "    commit_width : " << m_commit_width << std::endl;
   cout << "    fp_issue_width : " << m_fp_issue_width << std::endl;
   cout << "    prediction_width : " << m_prediction_width << std::endl;
   cout << "    integer_pipeline_depth : " << m_integer_pipeline_depth << std::endl;
   cout << "    fp_pipeline_depth : " << m_fp_pipeline_depth << std::endl;
   cout << "    ALU_per_core : " << m_ALU_per_core << std::endl;
   cout << "    MUL_per_core : " << m_MUL_per_core << std::endl;
   cout << "    FPU_per_core : " << m_FPU_per_core << std::endl;
   cout << "    instruction_buffer_size : " << m_instruction_buffer_size << std::endl;
   cout << "    decoded_stream_buffer_size : " << m_decoded_stream_buffer_size << std::endl;
   // |---- Register File
   cout << "    arch_regs_IRF_size : " << m_arch_regs_IRF_size << std::endl;
   cout << "    arch_regs_FRF_size : " << m_arch_regs_FRF_size << std::endl;
   cout << "    phy_regs_IRF_size : " << m_phy_regs_IRF_size << std::endl;
   cout << "    phy_regs_FRF_size : " << m_phy_regs_FRF_size << std::endl;
   // |---- Load-Store Unit
   cout << "    LSU_order : " << m_LSU_order << std::endl;
   cout << "    store_buffer_size : " << m_store_buffer_size << std::endl;
   cout << "    load_buffer_size : " << m_load_buffer_size << std::endl;
   cout << "    num_memory_ports : " << m_num_memory_ports << std::endl;
   cout << "    RAS_size : " << m_RAS_size << std::endl;
   // |---- OoO Core
   /*cout << "    instruction_window_scheme : " << m_instruction_window_scheme << std::endl;
   cout << "    instruction_window_size : " << m_instruction_window_size << std::endl;
   cout << "    fp_instruction_window_size : " << m_fp_instruction_window_size << std::endl;
   cout << "    ROB_size : " << m_ROB_size << std::endl;
   cout << "    rename_scheme : " << m_rename_scheme << std::endl;
   // |---- Register Windows (specific to Sun processors)
   cout << "    register_windows_size : " << m_register_windows_size << std::endl;*/
}

//---------------------------------------------------------------------------
// Display Event Counters
//---------------------------------------------------------------------------
void McPATCoreInterface::displayStats()
{
   // Event Counters
   cout << "  Core Statistics:" << std::endl;
   // |-- Used Event Counters
   // |---- Instruction Counters
   cout << "    total_instructions : " << m_total_instructions << std::endl;
   cout << "    int_instructions : " << m_int_instructions << std::endl;
   cout << "    fp_instructions : " << m_fp_instructions << std::endl;
   cout << "    branch_instructions : " << m_branch_instructions << std::endl;
   cout << "    branch_mispredictions : " << m_branch_mispredictions << std::endl;
   cout << "    load_instructions : " << m_load_instructions << std::endl;
   cout << "    store_instructions : " << m_store_instructions << std::endl;
   cout << "    committed_instructions : " << m_committed_instructions << std::endl;
   cout << "    committed_int_instructions : " << m_committed_int_instructions << std::endl;
   cout << "    committed_fp_instructions : " << m_committed_fp_instructions << std::endl;
   // |---- Cycle Counters
   cout << "    total_cycles : " << m_total_cycles << std::endl;
   cout << "    idle_cycles : " << m_idle_cycles << std::endl;
   cout << "    busy_cycles : " << m_busy_cycles << std::endl;
   // |---- Reg File Access Counters
   cout << "    int_regfile_reads : " << m_int_regfile_reads << std::endl;
   cout << "    int_regfile_writes : " << m_int_regfile_writes << std::endl;
   cout << "    fp_regfile_reads : " << m_fp_regfile_reads << std::endl;
   cout << "    fp_regfile_writes : " << m_fp_regfile_writes << std::endl;
   // |---- Execution Unit Access Counters
   cout << "    ialu_accesses : " << m_ialu_accesses << std::endl;
   cout << "    mul_accesses : " << m_mul_accesses << std::endl;
   cout << "    fpu_accesses : " << m_fpu_accesses << std::endl;
   cout << "    cdb_alu_accesses : " << m_cdb_alu_accesses << std::endl;
   cout << "    cdb_mul_accesses : " << m_cdb_mul_accesses << std::endl;
   cout << "    cdb_fpu_accesses : " << m_cdb_fpu_accesses << std::endl;
   /*// |-- Unused Event Counters
   // |---- OoO Core Event Counters
   cout << "    inst_window_reads : " << m_inst_window_reads << std::endl;
   cout << "    inst_window_writes : " << m_inst_window_writes << std::endl;
   cout << "    inst_window_wakeup_accesses : " << m_inst_window_wakeup_accesses << std::endl;
   cout << "    fp_inst_window_reads : " << m_fp_inst_window_reads << std::endl;
   cout << "    fp_inst_window_writes : " << m_fp_inst_window_writes << std::endl;
   cout << "    fp_inst_window_wakeup_accesses : " << m_fp_inst_window_wakeup_accesses << std::endl;
   cout << "    ROB_reads : " << m_ROB_reads << std::endl;
   cout << "    ROB_writes : " << m_ROB_writes << std::endl;
   cout << "    rename_accesses : " << m_rename_accesses << std::endl;
   cout << "    fp_rename_accesses : " << m_fp_rename_accesses << std::endl;
   // |---- Function Calls and Context Switches
   cout << "    function_calls : " << m_function_calls << std::endl;
   cout << "    context_switches : " << m_context_switches << std::endl;*/
}

//---------------------------------------------------------------------------
// Set Event Counters A
//---------------------------------------------------------------------------
void McPATCoreInterface::setEventCountersA()
{
   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   m_total_instructions = 800000;
   m_int_instructions = 600000;
   m_fp_instructions = 20000;
   m_branch_instructions = 0;
   m_branch_mispredictions = 0;
   m_load_instructions = 100000;
   m_store_instructions = 100000;
   m_committed_instructions = 800000;
   m_committed_int_instructions = 600000;
   m_committed_fp_instructions = 20000;
   // |---- Cycle Counters
   m_total_cycles = 100000;//100000;
   m_idle_cycles = 0;
   m_busy_cycles = 100000;//100000;
   // |---- Reg File Access Counters
   m_int_regfile_reads = 1600000;
   m_int_regfile_writes = 800000;
   m_fp_regfile_reads = 40000;
   m_fp_regfile_writes = 20000;
   // |---- Execution Unit Access Counters
   m_ialu_accesses = 800000;
   m_mul_accesses = 100000;
   m_fpu_accesses = 10000;
   m_cdb_alu_accesses = 1000000;
   m_cdb_mul_accesses = 0;
   m_cdb_fpu_accesses = 0;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   m_inst_window_reads = 263886;
   m_inst_window_writes = 263886;
   m_inst_window_wakeup_accesses = 263886;
   m_fp_inst_window_reads = 263886;
   m_fp_inst_window_writes = 263886;
   m_fp_inst_window_wakeup_accesses = 263886;
   m_ROB_reads = 263886;
   m_ROB_writes = 263886;
   m_rename_accesses = 263886;
   m_fp_rename_accesses = 263886;
   // |---- Function Calls and Context Switches
   m_function_calls = 5;
   m_context_switches = 260343;
}

//---------------------------------------------------------------------------
// Set Event Counters B
//---------------------------------------------------------------------------
void McPATCoreInterface::setEventCountersB()
{
   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   m_total_instructions = 800000;
   m_int_instructions = 600000;
   m_fp_instructions = 20000;
   m_branch_instructions = 0;
   m_branch_mispredictions = 0;
   m_load_instructions = 100000;
   m_store_instructions = 100000;
   m_committed_instructions = 800000;
   m_committed_int_instructions = 600000;
   m_committed_fp_instructions = 20000;
   // |---- Cycle Counters
   m_total_cycles = 100000;//100000;
   m_idle_cycles = 0;
   m_busy_cycles = 100000;//100000;
   // |---- Reg File Access Counters
   m_int_regfile_reads = 1600000;
   m_int_regfile_writes = 800000;
   m_fp_regfile_reads = 40000;
   m_fp_regfile_writes = 20000;
   // |---- Execution Unit Access Counters
   m_ialu_accesses = 400000;//800000;
   m_mul_accesses = 100000;
   m_fpu_accesses = 10000;
   m_cdb_alu_accesses = 1000000;
   m_cdb_mul_accesses = 0;
   m_cdb_fpu_accesses = 0;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   m_inst_window_reads = 263886;
   m_inst_window_writes = 263886;
   m_inst_window_wakeup_accesses = 263886;
   m_fp_inst_window_reads = 263886;
   m_fp_inst_window_writes = 263886;
   m_fp_inst_window_wakeup_accesses = 263886;
   m_ROB_reads = 263886;
   m_ROB_writes = 263886;
   m_rename_accesses = 263886;
   m_fp_rename_accesses = 263886;
   // |---- Function Calls and Context Switches
   m_function_calls = 5;
   m_context_switches = 260343;
}

//---------------------------------------------------------------------------
// Set Event Counters C
//---------------------------------------------------------------------------
void McPATCoreInterface::setEventCountersC()
{
   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   m_total_instructions = 800000;
   m_int_instructions = 600000;
   m_fp_instructions = 20000;
   m_branch_instructions = 0;
   m_branch_mispredictions = 0;
   m_load_instructions = 100000;
   m_store_instructions = 100000;
   m_committed_instructions = 800000;
   m_committed_int_instructions = 600000;
   m_committed_fp_instructions = 20000;
   // |---- Cycle Counters
   m_total_cycles = 50000;//100000;
   m_idle_cycles = 0;
   m_busy_cycles = 50000;//100000;
   // |---- Reg File Access Counters
   m_int_regfile_reads = 1600000;
   m_int_regfile_writes = 800000;
   m_fp_regfile_reads = 40000;
   m_fp_regfile_writes = 20000;
   // |---- Execution Unit Access Counters
   m_ialu_accesses = 800000;
   m_mul_accesses = 100000;
   m_fpu_accesses = 10000;
   m_cdb_alu_accesses = 1000000;
   m_cdb_mul_accesses = 0;
   m_cdb_fpu_accesses = 0;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   m_inst_window_reads = 263886;
   m_inst_window_writes = 263886;
   m_inst_window_wakeup_accesses = 263886;
   m_fp_inst_window_reads = 263886;
   m_fp_inst_window_writes = 263886;
   m_fp_inst_window_wakeup_accesses = 263886;
   m_ROB_reads = 263886;
   m_ROB_writes = 263886;
   m_rename_accesses = 263886;
   m_fp_rename_accesses = 263886;
   // |---- Function Calls and Context Switches
   m_function_calls = 5;
   m_context_switches = 260343;
}

}
