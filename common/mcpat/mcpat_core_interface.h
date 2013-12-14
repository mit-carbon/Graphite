/*****************************************************************************
 * Graphite-McPAT Core Interface
 ***************************************************************************/

#pragma once

#include <map>
using std::map;
#include "fixed_types.h"
#include "instruction.h"
#include "mcpat_instruction.h"
#include "contrib/mcpat/mcpat.h"

class CoreModel;

//---------------------------------------------------------------------------
// McPAT Core Interface Data Structures for Area and Power
//---------------------------------------------------------------------------
typedef struct
{
   double area;                           // Area
   double leakage_energy;                 // (Subthreshold + Gate) Leakage Energy
   double dynamic_energy;                 // Runtime Dynamic Energy
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
   mcpat_component_out BPT;               // Branch Prredictor Table
   mcpat_component_out BTB;               // Branch Target Buffer
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
   
   // McPAT Core Interface Constructor
   McPATCoreInterface(CoreModel* core_model, double frequency, double voltage, UInt32 load_buffer_size, UInt32 store_buffer_size);
   // McPAT Core Interface Destructor
   ~McPATCoreInterface();

   // Output summary
   void outputSummary(ostream& os, const Time& target_completion_time, double frequency);

   // Set DVFS
   void setDVFS(double old_frequency, double new_voltage, double new_frequency, const Time& curr_time);

   // Update Event Counters
   void updateEventCounters(const McPATInstruction* instruction, UInt64 cycle_count,
                            UInt64 total_branch_misprediction_count);
   // Compute Energy from McPat
   void computeEnergy(const Time& curr_time, double frequency);

   // Collect Energy from McPAT
   double getDynamicEnergy();
   double getLeakageEnergy();

private:
   CoreModel* _core_model;
   // McPAT Objects
   typedef map<double,McPAT::CoreWrapper*> CoreWrapperMap;
   CoreWrapperMap _core_wrapper_map;
   McPAT::CoreWrapper* _core_wrapper;
   McPAT::ParseXML* _xml;
   // Output Data Structure
   mcpat_core_output _mcpat_core_out;
   // Last Energy Compute Time
   Time _last_energy_compute_time;

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
   int _FPU_per_core;
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

   // Current Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   UInt64 _total_instructions;
   UInt64 _generic_instructions;
   UInt64 _int_instructions;
   UInt64 _fp_instructions;
   UInt64 _branch_instructions;
   UInt64 _branch_mispredictions;
   UInt64 _load_instructions;
   UInt64 _store_instructions;
   UInt64 _committed_instructions;
   UInt64 _committed_int_instructions;
   UInt64 _committed_fp_instructions;
   // |---- Reg File Access Counters
   UInt64 _int_regfile_reads;
   UInt64 _int_regfile_writes;
   UInt64 _fp_regfile_reads;
   UInt64 _fp_regfile_writes;
   // |---- Execution Unit Access Counters
   UInt64 _ialu_accesses;
   UInt64 _mul_accesses;
   UInt64 _fpu_accesses;
   UInt64 _cdb_alu_accesses;
   UInt64 _cdb_mul_accesses;
   UInt64 _cdb_fpu_accesses;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   UInt64 _inst_window_reads;
   UInt64 _inst_window_writes;
   UInt64 _inst_window_wakeup_accesses;
   UInt64 _fp_inst_window_reads;
   UInt64 _fp_inst_window_writes;
   UInt64 _fp_inst_window_wakeup_accesses;
   UInt64 _ROB_reads;
   UInt64 _ROB_writes;
   UInt64 _rename_accesses;
   UInt64 _fp_rename_accesses;
   // |---- Function Calls and Context Switches
   UInt64 _function_calls;
   UInt64 _context_switches;

   // Prev Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   UInt64 _prev_instructions;
   UInt64 _prev_generic_instructions;
   UInt64 _prev_int_instructions;
   UInt64 _prev_fp_instructions;
   UInt64 _prev_branch_instructions;
   UInt64 _prev_branch_mispredictions;
   UInt64 _prev_load_instructions;
   UInt64 _prev_store_instructions;
   UInt64 _prev_committed_instructions;
   UInt64 _prev_committed_int_instructions;
   UInt64 _prev_committed_fp_instructions;
   // |---- Reg File Access Counters
   UInt64 _prev_int_regfile_reads;
   UInt64 _prev_int_regfile_writes;
   UInt64 _prev_fp_regfile_reads;
   UInt64 _prev_fp_regfile_writes;
   // |---- Execution Unit Access Counters
   UInt64 _prev_ialu_accesses;
   UInt64 _prev_mul_accesses;
   UInt64 _prev_fpu_accesses;
   UInt64 _prev_cdb_alu_accesses;
   UInt64 _prev_cdb_mul_accesses;
   UInt64 _prev_cdb_fpu_accesses;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   UInt64 _prev_inst_window_reads;
   UInt64 _prev_inst_window_writes;
   UInt64 _prev_inst_window_wakeup_accesses;
   UInt64 _prev_fp_inst_window_reads;
   UInt64 _prev_fp_inst_window_writes;
   UInt64 _prev_fp_inst_window_wakeup_accesses;
   UInt64 _prev_ROB_reads;
   UInt64 _prev_ROB_writes;
   UInt64 _prev_rename_accesses;
   UInt64 _prev_fp_rename_accesses;
   // |---- Function Calls and Context Switches
   UInt64 _prev_function_calls;
   UInt64 _prev_context_switches;
  
   bool _enable_area_or_power_modeling;

   // Update Event Counters
   void updateInstructionCounters(const McPATInstruction* instruction);
   void updateRegFileAccessCounters(const McPATInstruction* instruction);
   void updateExecutionUnitCounters(const McPATInstruction* instruction);

   // Create core wrapper
   McPAT::CoreWrapper* createCoreWrapper(double voltage, double max_frequency_at_voltage);
   // Initialize XML Object
   void fillCoreParamsIntoXML(UInt32 technology_node, UInt32 temperature);
   void fillCoreStatsIntoXML(UInt64 interval_cycles);
   
   // Initialize Architectural Parameters
   void initializeArchitecturalParameters(UInt32 load_queue_size, UInt32 store_queue_size);
   // Initialize Event Counters
   void initializeEventCounters();
   // Initialize/update Output Data Structure
   void initializeOutputDataStructure();
   void updateOutputDataStructure(double time_interval);
   
   // Display Energy from McPAT
   void displayEnergy(ostream& os, const Time& target_completion_time);
   // Display Architectural Parameters
   void displayParam(ostream& os);
   // Display Event Counters
   void displayStats(ostream& os);
};
