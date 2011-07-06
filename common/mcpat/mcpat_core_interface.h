#pragma once

#include <vector>
#include <string>
using std::vector;
using std::string;

#include "fixed_types.h"
#include "instruction.h"

class McPATCoreInterface
{
public:
   enum InstructionType
   {
      INTEGER_INST = 0,
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

   McPATCoreInterface(UInt32 load_buffer_size, UInt32 store_buffer_size);
   ~McPATCoreInterface();
   
   // Update Event Counters
   void updateEventCounters(Instruction* instruction, UInt64 cycle_count);

private:
   // Architectural Parameters
   // |---- General Parameters
   UInt32 m_instruction_length;
   UInt32 m_opcode_width;
   UInt32 m_machine_type;
   UInt32 m_num_hardware_threads;
   UInt32 m_fetch_width;
   UInt32 m_num_instruction_fetch_ports;
   UInt32 m_decode_width;
   UInt32 m_issue_width;
   UInt32 m_commit_width;
   UInt32 m_fp_issue_width;
   UInt32 m_prediction_width;
   UInt32 m_integer_pipeline_depth;
   UInt32 m_fp_pipeline_depth;
   UInt32 m_ALU_per_core;
   UInt32 m_MUL_per_core;
   UInt32 m_FPU_per_core;
   UInt32 m_instruction_buffer_size;
   UInt32 m_decoded_stream_buffer_size;
   // |---- Register File
   UInt32 m_arch_regs_IRF_size;
   UInt32 m_arch_regs_FRF_size;
   UInt32 m_phy_regs_IRF_size;
   UInt32 m_phy_regs_FRF_size;
   // |---- Load-Store Unit
   string m_LSU_order;
   UInt32 m_store_buffer_size;
   UInt32 m_load_buffer_size;
   UInt32 m_num_memory_ports;
   UInt32 m_RAS_size;
   // |---- OoO Core
   UInt32 m_instruction_window_scheme;
   UInt32 m_instruction_window_size;
   UInt32 m_fp_instruction_window_size;
   UInt32 m_ROB_size;
   UInt32 m_rename_scheme;
   // |---- Register Windows (specific to Sun processors)
   UInt32 m_register_windows_size;
   
   // Event Counters
   // |-- Used Event Counters
   // |---- Instruction Counters
   UInt64 m_total_instructions;
   UInt64 m_int_instructions;
   UInt64 m_fp_instructions;
   UInt64 m_branch_instructions;
   UInt64 m_branch_mispredictions;
   UInt64 m_load_instructions;
   UInt64 m_store_instructions;
   UInt64 m_committed_instructions;
   UInt64 m_committed_int_instructions;
   UInt64 m_committed_fp_instructions;
   // |---- Cycle Counters
   UInt64 m_total_cycles;
   UInt64 m_idle_cycles;
   UInt64 m_busy_cycles;
   // |---- Reg File Access Counters
   UInt64 m_int_regfile_reads;
   UInt64 m_int_regfile_writes;
   UInt64 m_fp_regfile_reads;
   UInt64 m_fp_regfile_writes;
   // |---- Execution Unit Access Counters
   UInt64 m_ialu_accesses;
   UInt64 m_mul_accesses;
   UInt64 m_fpu_accesses;
   UInt64 m_cdb_alu_accesses;
   UInt64 m_cdb_mul_accesses;
   UInt64 m_cdb_fpu_accesses;
   // |-- Unused Event Counters
   // |---- OoO Core Event Counters
   UInt64 m_inst_window_reads;
   UInt64 m_inst_window_writes;
   UInt64 m_inst_window_wakeup_accesses;
   UInt64 m_fp_inst_window_reads;
   UInt64 m_fp_inst_window_writes;
   UInt64 m_fp_inst_window_wakeup_accesses;
   UInt64 m_ROB_reads;
   UInt64 m_ROB_writes;
   UInt64 m_rename_accesses;
   UInt64 m_fp_rename_accesses;
   // |---- Function Calls and Context Switches
   UInt64 m_function_calls;
   UInt64 m_context_switches;

   // Initialize Architectural Parameters
   void initializeArchitecturalParameters(UInt32 load_buffer_size, UInt32 store_buffer_size);
   void initializeRegFileParameters();
   void initializeLoadStoreUnitParameters(UInt32 load_buffer_size, UInt32 store_buffer_size);
   void initializeOoOCoreParameters();
   
   // Initialize Event Counters
   void initializeEventCounters();
   void initializeInstructionCounters();
   void initializeCycleCounters();
   void initializeRegFileAccessCounters();
   void initializeExecutionUnitAccessCounters();
   void initializeOoOEventCounters();
   void initializeMiscEventCounters();
   
   // Update Event Counters
   void updateInstructionCounters(InstructionType instruction_type);
   void updateRegFileAccessCounters(Operand::Direction operand_direction, UInt32 reg_id);
   void updateExecutionUnitAccessCounters(ExecutionUnitType unit_type);
   void updateCycleCounters(UInt64 cycle_count);
};

McPATCoreInterface::InstructionType getInstructionType(UInt64 opcode);
McPATCoreInterface::ExecutionUnitList getExecutionUnitAccessList(UInt64 opcode);
bool isIntegerReg(UInt32 reg_id);
bool isFloatingPointReg(UInt32 reg_id);
