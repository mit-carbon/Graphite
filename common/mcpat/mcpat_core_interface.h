/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#pragma once

#include <stdio.h>
#include <string.h>
#include <iostream>
#include "fixed_types.h"
#include "instruction.h"
#include "contrib/mcpat/mcpat.h"

using namespace std;

//---------------------------------------------------------------------------
// McPAT Core Interface Data Structures for Area and Power
//---------------------------------------------------------------------------
typedef struct
{
   double area;                           // Area
   double leakage;                        // Subthreshold Leakage
   double longer_channel_leakage;         // Subthreshold Leakage
   double gate_leakage;                   // Gate Leakage
   double dynamic;                        // Runtime Dynamic
} mcpat_component_out;
typedef struct
{
   mcpat_component_out rfu;               // Register Files
   mcpat_component_out IRF;               // Integer RF
   mcpat_component_out FRF;               // Floating Point RF
   mcpat_component_out RFWIN;             // Register Windows
} mcpat_rfu_out;
typedef struct
{
   mcpat_component_out scheu;             // Instruction Scheduler
   mcpat_component_out int_inst_window;   // Instruction Window
} mcpat_scheu_out;
typedef struct
{
   mcpat_component_out ifu;               // Instruction Fetch Unit
   mcpat_component_out icache;            // Instruction Cache
   mcpat_component_out IB;                // Instruction Buffer
   mcpat_component_out ID;                // Instruction Decoder
} mcpat_ifu_out;
typedef struct
{
   mcpat_component_out lsu;               // Load Store Unit
   mcpat_component_out dcache;            // Data Cache
   mcpat_component_out LSQ;               // Load/Store Queue
} mcpat_lsu_out;
typedef struct
{
   mcpat_component_out mmu;               // Memory Management Unit
   mcpat_component_out itlb;              // Itlb
   mcpat_component_out dtlb;              // Dtlb
} mcpat_mmu_out;
typedef struct
{
   mcpat_component_out exu;               // Execution Unit
   mcpat_rfu_out rfu;                     // Register Files
   mcpat_scheu_out scheu;                 // Instruction Scheduler
   mcpat_component_out exeu;              // Integer ALUs
   mcpat_component_out fp_u;              // Floating Point Units (FPUs)
   mcpat_component_out mul;               // Complex ALUs (Mul/Div)
   mcpat_component_out bypass;            // Results Broadcast Bus
} mcpat_exu_out;
typedef struct
{
   mcpat_component_out core;              // Core
   mcpat_ifu_out ifu;                     // Instruction Fetch Unit
   mcpat_lsu_out lsu;                     // Load Store Unit
   mcpat_mmu_out mmu;                     // Memory Management Unit
   mcpat_exu_out exu;                     // Execution Unit
} mcpat_core_output;

//---------------------------------------------------------------------------
// McPAT Core Interface
//---------------------------------------------------------------------------
class McPATCoreInterface
{
public:
   enum McPATInstructionType
   {
      GENERIC_INST = 0,
      INTEGER_INST,
      FLOATING_POINT_INST,
      LOAD_INST,
      STORE_INST,
      BRANCH_INST,
      BRANCH_NOT_TAKEN_INST
   };
   enum ExecutionUnitType
   {
      ALU = 0,
      MUL,
      FPU
   };
   typedef vector<ExecutionUnitType> ExecutionUnitList;
   
   // McPAT Core Interface Constructor
   McPATCoreInterface(UInt32 technology_node, UInt32 core_frequency, UInt32 load_buffer_size, UInt32 store_buffer_size);
   // McPAT Core Interface Destructor
   ~McPATCoreInterface();

   // Update Event Counters
   void updateEventCounters(Instruction* instruction, UInt64 cycle_count);
   
   // Compute Energy from McPat
   void computeMcPATCoreEnergy();

   // Display Energy from McPat
   void displayMcPATCoreEnergy(ostream& os);
   // Display Architectural Parameters
   void displayParam(ostream& os);
   // Display Event Counters
   void displayStats(ostream& os);

private:
   // McPAT Objects
   McPAT::ParseXML* _xml;
   McPAT::CoreWrapper* _core_wrapper;
   // McPAT Output Data Structure
   mcpat_core_output _mcpat_core_out;

   // System Parameters
   // Execution Time
   double _execution_time;
   // |---- General Parameters
   int _clock_rate;
   double _core_tech_node;
   // Architectural Parameters
   // |---- General Parameters
   int _instruction_length;
   int _opcode_width;
   int _machine_type;
   int _num_hardware_threads;
   int _fetch_width;
   int _num_instruction_fetch_ports;
   int _decode_width;
   int _issue_width;
   int _commit_width;
   int _fp_issue_width;
   int _prediction_width;
   int _integer_pipeline_depth;
   int _fp_pipeline_depth;
   int _ALU_per_core;
   int _MUL_per_core;
   double _FPU_per_core;
   int _instruction_buffer_size;
   int _decoded_stream_buffer_size;
   // |---- Register File
   int _arch_regs_IRF_size;
   int _arch_regs_FRF_size;
   int _phy_regs_IRF_size;
   int _phy_regs_FRF_size;
   // |---- Load-Store Unit
   string _LSU_order;
   int _store_buffer_size;
   int _load_buffer_size;
   int _num_memory_ports;
   int _RAS_size;
   // |---- OoO Core
   int _instruction_window_scheme;
   int _instruction_window_size;
   int _fp_instruction_window_size;
   int _ROB_size;
   int _rename_scheme;
   // |---- Register Windows (specific to Sun processors)
   int _register_windows_size;

   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   double _total_instructions;
   double _int_instructions;
   double _fp_instructions;
   double _branch_instructions;
   double _branch_mispredictions;
   double _load_instructions;
   double _store_instructions;
   double _committed_instructions;
   double _committed_int_instructions;
   double _committed_fp_instructions;
   // |---- Cycle Counters
   double _total_cycles;
   double _idle_cycles;
   double _busy_cycles;
   // |---- Reg File Access Counters
   double _int_regfile_reads;
   double _int_regfile_writes;
   double _fp_regfile_reads;
   double _fp_regfile_writes;
   // |---- Execution Unit Access Counters
   double _ialu_accesses;
   double _mul_accesses;
   double _fpu_accesses;
   double _cdb_alu_accesses;
   double _cdb_mul_accesses;
   double _cdb_fpu_accesses;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   double _inst_window_reads;
   double _inst_window_writes;
   double _inst_window_wakeup_accesses;
   double _fp_inst_window_reads;
   double _fp_inst_window_writes;
   double _fp_inst_window_wakeup_accesses;
   double _ROB_reads;
   double _ROB_writes;
   double _rename_accesses;
   double _fp_rename_accesses;
   // |---- Function Calls and Context Switches
   double _function_calls;
   double _context_switches;
   
   bool _enable_area_and_power_modeling;

   // Initialize Architectural Parameters
   void initializeArchitecturalParameters(UInt32 technology_node, UInt32 core_frequency, UInt32 load_buffer_size, UInt32 store_buffer_size);
   // Initialize Event Counters
   void initializeEventCounters();
   // Initialize Output Data Structure
   void initializeOutputDataStructure();

   // Update Event Counters
   void updateInstructionCounters(McPATInstructionType instruction_type);
   void updateRegFileAccessCounters(Operand::Direction operand_direction, UInt32 reg_id);
   void updateExecutionUnitAccessCounters(ExecutionUnitType unit_type);
   void updateCycleCounters(UInt64 cycle_count);

   // Initialize XML Object
   void fillCoreParamsIntoXML();
   void fillCoreStatsIntoXML();
};

McPATCoreInterface::McPATInstructionType getMcPATInstructionType(InstructionType type);
McPATCoreInterface::ExecutionUnitList getExecutionUnitAccessList(InstructionType type);
bool isIntegerReg(UInt32 reg_id);
bool isFloatingPointReg(UInt32 reg_id);
bool isXMMReg(UInt32 reg_id);
