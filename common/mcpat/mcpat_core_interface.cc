#include "mcpat_core_interface.h"
#include "log.h"

McPATCoreInterface::McPATCoreInterface(UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // Initialize Architectural Paramaters
   initializeArchitecturalParameters(load_buffer_size, store_buffer_size);

   // Initialize Event Counters
   initializeEventCounters();
}

McPATCoreInterface::~McPATCoreInterface()
{}

void McPATCoreInterface::initializeArchitecturalParameters(UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // I have retained the same values from McPAT here
   // x86 instructions have variable length and variable opcode length too
   m_instruction_length = 32;
   m_opcode_width = 9;

   // In-order core
   m_machine_type = 1;

   // 1 hardware thread/core
   m_num_hardware_threads = 1;

   // Instruction Fetch Width (defaults from McPAT)
   m_fetch_width = 1;
   m_num_instruction_fetch_ports = 1;

   // Decode Width (defaults from McPAT)
   m_decode_width = 1;

   // Issue Width
   m_issue_width = 1;
   m_fp_issue_width = 1;

   // Commit Width
   m_commit_width = 1;

   // Default from McPAT
   m_prediction_width = 0;

   // Pipeline Depths
   m_integer_pipeline_depth = 6;
   m_fp_pipeline_depth = 6;

   // Number of Execution Units
   m_ALU_per_core = 1;
   m_MUL_per_core = 1;
   m_FPU_per_core = 1;

   // Buffers between pipeline stages
   // Defaults from McPAT
   m_instruction_buffer_size = 16;
   m_decoded_stream_buffer_size = 16;
   
   initializeRegFileParameters();
   initializeLoadStoreUnitParameters(load_buffer_size, store_buffer_size);
   initializeOoOCoreParameters();
}

void McPATCoreInterface::initializeRegFileParameters()
{
   // Reg File (hard-coded for x86-64 architecture)
   // General Purpose Registers - RIP,RFLAGS,RAX,RBX,RCX,RDX,RSI,RDI,RBP,RSP,R8,R9,R10,R11,R12,R13,R14,R15
   // Segment Registers - CS,SS,DS,ES,FS,GS
   m_arch_regs_IRF_size = 24;
   // Floating Point Registers - ST0-ST7
   // SSE Registers - XMM0-XMM7 (Each SSE register treated as 2 floating point registers)
   m_arch_regs_FRF_size = 24;

   // Number of Physical Registers same as number of architectural registers currently
   m_phy_regs_IRF_size = 24;
   m_phy_regs_FRF_size = 24;
}

void McPATCoreInterface::initializeLoadStoreUnitParameters(UInt32 load_buffer_size, UInt32 store_buffer_size)
{
   // Load/Store Unit
   m_LSU_order = "inorder";
   m_load_buffer_size = load_buffer_size;
   m_store_buffer_size = store_buffer_size;
   m_num_memory_ports = 1;
   m_RAS_size = 16; // default from McPAT
}

void McPATCoreInterface::initializeOoOCoreParameters()
{
   m_instruction_window_scheme = 0;
   m_instruction_window_size = 0;
   m_fp_instruction_window_size = 0;
   m_ROB_size = 0;
   m_rename_scheme = 0;
   m_register_windows_size = 0;
}

void McPATCoreInterface::initializeEventCounters()
{
   initializeInstructionCounters();
   initializeCycleCounters();
   initializeRegFileAccessCounters();
   initializeExecutionUnitAccessCounters();
   initializeOoOEventCounters();
   initializeMiscEventCounters();
}

void McPATCoreInterface::initializeInstructionCounters()
{
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
}

void McPATCoreInterface::initializeCycleCounters()
{
   m_total_cycles = 0;
   m_idle_cycles = 0;
   m_busy_cycles = 0;
}

void McPATCoreInterface::initializeRegFileAccessCounters()
{
   m_int_regfile_reads = 0;
   m_int_regfile_writes = 0;
   m_fp_regfile_reads = 0;
   m_fp_regfile_writes = 0;
}

void McPATCoreInterface::initializeExecutionUnitAccessCounters()
{
   m_ialu_accesses = 0;
   m_mul_accesses = 0;
   m_fpu_accesses = 0;
   m_cdb_alu_accesses = 0;
   m_cdb_mul_accesses = 0;
   m_cdb_fpu_accesses = 0;
}

void McPATCoreInterface::initializeOoOEventCounters()
{
   // |---- OoO Core Event Counters
   // This is only for an input to McPAT
   // Instruction Windows in OoO processors
   m_inst_window_reads = 0;
   m_inst_window_writes = 0;
   m_inst_window_wakeup_accesses = 0;
   m_fp_inst_window_reads = 0;
   m_fp_inst_window_writes = 0;
   m_fp_inst_window_wakeup_accesses = 0;

   // Re-order buffers in OoO processors
   m_ROB_reads = 0;
   m_ROB_writes = 0;
   m_rename_accesses = 0;
   m_fp_rename_accesses = 0;
}

void McPATCoreInterface::initializeMiscEventCounters()
{
   // |---- Function Calls and Context Switches
   m_function_calls = 0;
   m_context_switches = 0;
}

void McPATCoreInterface::updateEventCounters(Instruction* instruction, UInt64 cycle_count)
{
   // Get Instruction Type
   InstructionType instruction_type = getInstructionType(instruction->getOpcode());
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
   ExecutionUnitList access_list = getExecutionUnitAccessList(instruction->getOpcode());
   for (UInt32 i = 0; i < access_list.size(); i++)
      updateExecutionUnitAccessCounters(access_list[i]);
}

void McPATCoreInterface::updateInstructionCounters(InstructionType instruction_type)
{
   m_total_instructions ++;
   m_committed_instructions ++;
   
   switch (instruction_type)
   {
   case INTEGER_INST:
      m_int_instructions ++;
      m_committed_instructions ++;
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

// Dummy Implementations

__attribute__((weak)) McPATCoreInterface::InstructionType
getInstructionType(UInt64 opcode)
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
getExecutionUnitAccessList(UInt64 opcode)
{
   McPATCoreInterface::ExecutionUnitList access_list;
   access_list.push_back(McPATCoreInterface::ALU);
   return access_list;
}
