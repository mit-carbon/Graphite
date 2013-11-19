#include "instruction_modeling.h"
#include "simulator.h"
#include "core_model.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "hash_map.h"
#include "mcpat_core_helper.h"

extern HashMap core_map;

void handleInstruction(THREADID thread_id, Instruction* instruction)
{
   if (!Sim()->isEnabled())
      return;

   CoreModel *core_model = core_map.get<Core>(thread_id)->getModel();
   core_model->queueInstruction(instruction);
   core_model->iterate();
}

void handleBranch(THREADID thread_id, BOOL taken, ADDRINT target)
{
   if (!Sim()->isEnabled())
      return;

   CoreModel *core_model = core_map.get<Core>(thread_id)->getModel();
   DynamicBranchInfo info(taken, target);
   core_model->pushDynamicBranchInfo(info);
}

void fillNumMemoryOperands(UInt32& num_read_memory_operands,
                           UInt32& num_write_memory_operands,
                           INS ins)
{
   // NOTE: This code is written to reflect rewriteStackOp and
   // rewriteMemOp etc from redirect_memory.cc and it MUST BE
   // MAINTAINED to reflect that code.

   if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
   {
      // stack ops
      if (INS_Opcode (ins) == XED_ICLASS_PUSH)
      {
         if (INS_OperandIsImmediate (ins, 0))
            num_write_memory_operands ++;
         
         else if (INS_OperandIsReg (ins, 0))
            num_write_memory_operands ++;

         else if (INS_OperandIsMemory (ins, 0))
         {
            num_read_memory_operands ++;
            num_write_memory_operands ++;
         }
      }
      
      else if (INS_Opcode (ins) == XED_ICLASS_POP)
      {
         if (INS_OperandIsReg (ins, 0))
            num_read_memory_operands ++;

         else if (INS_OperandIsMemory (ins, 0))
         {
            num_read_memory_operands ++;
            num_write_memory_operands ++;
         }
      }
     
      else if (INS_IsCall (ins))
      {
         if (INS_OperandIsMemory (ins, 0))
         {
            num_read_memory_operands ++;
            num_write_memory_operands ++;
         }

         else
            num_write_memory_operands ++;
      }

      else if (INS_IsRet (ins))
         num_read_memory_operands ++;

      else if (INS_Opcode (ins) == XED_ICLASS_LEAVE)
         num_read_memory_operands ++;

      else if ((INS_Opcode (ins) == XED_ICLASS_PUSHF) || (INS_Opcode (ins) == XED_ICLASS_PUSHFD))
         num_write_memory_operands ++;

      else if ((INS_Opcode (ins) == XED_ICLASS_POPF) || (INS_Opcode (ins) == XED_ICLASS_POPFD))
         num_read_memory_operands ++;
   
      // mem ops
      else if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
      {
         if (INS_IsMemoryRead (ins))
            num_read_memory_operands ++;

         if (INS_HasMemoryRead2 (ins))
            num_read_memory_operands ++;

         if (INS_IsMemoryWrite (ins))
            num_write_memory_operands ++;
      }
   }
   
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
   {
      // mem ops
      if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
      {
         if (INS_IsMemoryRead (ins))
            num_read_memory_operands ++;

         if (INS_HasMemoryRead2 (ins))
            num_read_memory_operands ++;

         if (INS_IsMemoryWrite (ins))
            num_write_memory_operands ++;
      }
   }
}

VOID fillRegisterOperandList(RegisterOperandList& read_register_operands,
                             RegisterOperandList& write_register_operands,
                             INS ins)
{
   // for handling register operands
   unsigned int max_read_regs = INS_MaxNumRRegs(ins);
   unsigned int max_write_regs = INS_MaxNumRRegs(ins);

   for (unsigned int i = 0; i < max_read_regs; i++)
   {
      if (REG_valid(INS_RegR(ins, i)))
         read_register_operands.push_back(INS_RegR(ins, i));
   }

   for (unsigned int i = 0; i < max_write_regs; i++)
   {
      if (REG_valid(INS_RegW(ins, i)))
         write_register_operands.push_back(INS_RegW(ins, i));
   }
}

VOID fillImmediateOperandList(ImmediateOperandList& immediate_operands, INS ins)
{
   // immediate
   for (unsigned int i = 0; i < INS_OperandCount(ins); i++)
   {
      if (INS_OperandIsImmediate(ins, i))
         immediate_operands.push_back(INS_OperandImmediate(ins, i));
   }
}

InstructionType getInstructionType(INS ins)
{
   OPCODE inst_opcode = INS_Opcode(ins);
  
   // BRANCH instruction 
   if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
   {
      return INST_BRANCH;
   }

   // MOV instruction
   else if 
      (
       (inst_opcode >= XED_ICLASS_MOV) && (inst_opcode <= XED_ICLASS_MOVZX)
      )
   {
      return INST_MOV;
   }

   // LFENCE
   else if (inst_opcode == XED_ICLASS_LFENCE)
   {
      return INST_LFENCE;
   }
   // SFENCE
   else if (inst_opcode == XED_ICLASS_SFENCE)
   {
      return INST_SFENCE;
   }
   // MFENCE
   else if (inst_opcode == XED_ICLASS_MFENCE)
   {
      return INST_MFENCE;
   }

   // FPU Opcode
   else if
      (
       (inst_opcode == XED_ICLASS_FADD) || (inst_opcode == XED_ICLASS_FADDP) ||
       (inst_opcode == XED_ICLASS_FIADD) ||
       (inst_opcode == XED_ICLASS_FSUB) || (inst_opcode == XED_ICLASS_FSUBP) ||
       (inst_opcode == XED_ICLASS_FISUB) ||
       (inst_opcode == XED_ICLASS_FSUBR) || (inst_opcode == XED_ICLASS_FSUBRP) ||
       (inst_opcode == XED_ICLASS_FISUBR)
      )
   {
      return INST_FALU;
   }
   else if 
      (
       (inst_opcode == XED_ICLASS_FMUL) || (inst_opcode == XED_ICLASS_FMULP)||
       (inst_opcode == XED_ICLASS_FIMUL)
      )
   {
      return INST_FMUL;
   }
   else if
      (
       (inst_opcode == XED_ICLASS_FDIV) || (inst_opcode == XED_ICLASS_FDIVP)||
       (inst_opcode == XED_ICLASS_FDIVR) || (inst_opcode == XED_ICLASS_FDIVRP)||
       (inst_opcode == XED_ICLASS_FIDIV) || (inst_opcode == XED_ICLASS_FIDIVR)
      )
   {
      return INST_FDIV;
   }

   // SIMD Instructions
   else if
      (
       (inst_opcode == XED_ICLASS_ADDSS) ||
       (inst_opcode == XED_ICLASS_SUBSS) ||
       (inst_opcode == XED_ICLASS_MULSS) ||
       (inst_opcode == XED_ICLASS_DIVSS) ||
       (inst_opcode == XED_ICLASS_MINSS) ||
       (inst_opcode == XED_ICLASS_MAXSS) ||
       (inst_opcode == XED_ICLASS_SQRTSS) ||
       (inst_opcode == XED_ICLASS_RSQRTSS) ||
       (inst_opcode == XED_ICLASS_RCPSS) ||
       (inst_opcode == XED_ICLASS_CMPSS) ||
       (inst_opcode == XED_ICLASS_COMISS)
      )
   {
      return INST_XMM_SS;
   }
   else if
      (
       (inst_opcode == XED_ICLASS_ADDSD) ||
       (inst_opcode == XED_ICLASS_SUBSD) ||
       (inst_opcode == XED_ICLASS_MULSD) ||
       (inst_opcode == XED_ICLASS_DIVSD) ||
       (inst_opcode == XED_ICLASS_MINSD) ||
       (inst_opcode == XED_ICLASS_MAXSD) ||
       (inst_opcode == XED_ICLASS_SQRTSD)
      )
   {
      return INST_XMM_SD;
   }
   else if
      (
       (inst_opcode == XED_ICLASS_ADDPS) || (inst_opcode == XED_ICLASS_ADDPD) ||
       (inst_opcode == XED_ICLASS_SUBPS) || (inst_opcode == XED_ICLASS_SUBPD) ||
       (inst_opcode == XED_ICLASS_MULPS) || (inst_opcode == XED_ICLASS_MULPD) ||
       (inst_opcode == XED_ICLASS_DIVPS) || (inst_opcode == XED_ICLASS_DIVPD) ||
       (inst_opcode == XED_ICLASS_MINPS) || (inst_opcode == XED_ICLASS_MINPD) ||
       (inst_opcode == XED_ICLASS_MAXPS) || (inst_opcode == XED_ICLASS_MAXPD) ||
       (inst_opcode == XED_ICLASS_SQRTPS) || (inst_opcode == XED_ICLASS_SQRTPD) ||
       (inst_opcode == XED_ICLASS_RSQRTPS) ||
       (inst_opcode == XED_ICLASS_RCPPS) || (inst_opcode == XED_ICLASS_CMPPS)
      )
   {
      return INST_XMM_PS;
   }
 
   // Integer Addition
   else if
     (
      (inst_opcode == XED_ICLASS_ADD) || (inst_opcode == XED_ICLASS_ADC) ||
      (inst_opcode == XED_ICLASS_PADDB) ||
      (inst_opcode == XED_ICLASS_PADDW) || (inst_opcode == XED_ICLASS_PADDD) ||
      (inst_opcode == XED_ICLASS_PADDSB) || (inst_opcode == XED_ICLASS_PADDSW) ||
      (inst_opcode == XED_ICLASS_PADDUSB) || (inst_opcode == XED_ICLASS_PADDUSW)
     )
   {
      return INST_IALU;
   }

   // Integer Subtraction
   else if
     (
      (inst_opcode == XED_ICLASS_SUB) ||
      (inst_opcode == XED_ICLASS_PSUBB) || (inst_opcode == XED_ICLASS_PSUBW) ||
      (inst_opcode == XED_ICLASS_PSUBD) ||
      (inst_opcode == XED_ICLASS_PSUBSB) || (inst_opcode == XED_ICLASS_PSUBSW) ||
      (inst_opcode == XED_ICLASS_PSUBUSB) || (inst_opcode == XED_ICLASS_PSUBUSW)
     ) 
   {
      return INST_IALU;
   }
         
   // Bitwise operations
   else if
      (
       (inst_opcode == XED_ICLASS_OR) || (inst_opcode == XED_ICLASS_AND) ||
       (inst_opcode == XED_ICLASS_XOR) || (inst_opcode == XED_ICLASS_ANDNPS)
      )
   {
      return INST_IALU;
   }

   // Other simple operations
   else if
      (
       (inst_opcode == XED_ICLASS_CMP) || (inst_opcode == XED_ICLASS_BSF) ||
       (inst_opcode == XED_ICLASS_BSR) || (inst_opcode == XED_ICLASS_BTC) ||
       (inst_opcode == XED_ICLASS_BTR) || (inst_opcode == XED_ICLASS_BTS) ||
       (inst_opcode == XED_ICLASS_CMPSB) ||
       (inst_opcode == XED_ICLASS_CMPSW) || (inst_opcode == XED_ICLASS_CMPSD) ||
       (inst_opcode == XED_ICLASS_CMPXCHG)
      )
   {
      return INST_IALU;
   }
         
   // Integer Multiplication 
   else if 
      (
       (inst_opcode == XED_ICLASS_MUL) || (inst_opcode == XED_ICLASS_IMUL)||
       (inst_opcode == XED_ICLASS_PCLMULQDQ) || (inst_opcode == XED_ICLASS_PFMUL)||
       (inst_opcode == XED_ICLASS_PMULDQ) || (inst_opcode == XED_ICLASS_PMULHRSW)||
       (inst_opcode == XED_ICLASS_PMULHRW) || (inst_opcode == XED_ICLASS_PMULHUW)||
       (inst_opcode == XED_ICLASS_PMULHW) || (inst_opcode == XED_ICLASS_PMULLD)||
       (inst_opcode == XED_ICLASS_PMULLW) || (inst_opcode == XED_ICLASS_PMULUDQ)
      )
   {
      return INST_IMUL;
   }

   // Integer Division
   else if 
      (
       (inst_opcode == XED_ICLASS_DIV) || (inst_opcode == XED_ICLASS_IDIV)
      )
   {
      return INST_IDIV;
   }

   // Generic Instruction with no functional unit
   else
   {
      return INST_GENERIC;
   }
}

VOID addInstructionModeling(INS ins)
{
   RegisterOperandList read_register_operands;
   RegisterOperandList write_register_operands;
   UInt32 num_read_memory_operands = 0;
   UInt32 num_write_memory_operands = 0;
   ImmediateOperandList immediate_operands;

   fillRegisterOperandList(read_register_operands, write_register_operands, ins);
   fillNumMemoryOperands(num_read_memory_operands, num_write_memory_operands, ins);
   fillImmediateOperandList(immediate_operands, ins);
   OperandList operand_list(read_register_operands, write_register_operands,
                            num_read_memory_operands, num_write_memory_operands,
                            immediate_operands);

   InstructionType instruction_type = getInstructionType(ins);
   McPATInstruction::MicroOpList micro_op_list;
   McPATInstruction::RegisterFile register_file;
   McPATInstruction::ExecutionUnitList execution_unit_list;

   fillMcPATMicroOpList(micro_op_list, instruction_type, num_read_memory_operands, num_write_memory_operands);
   fillMcPATRegisterFileAccessCounters(register_file, read_register_operands, write_register_operands);
   fillMcPATExecutionUnitList(execution_unit_list, instruction_type);
   McPATInstruction* mcpat_instruction = new McPATInstruction(micro_op_list, register_file, execution_unit_list);

   Instruction* instruction;

   // branches
   if (instruction_type == INST_BRANCH)
   {
      instruction = new BranchInstruction(INS_Opcode(ins), INS_Address(ins), INS_Size(ins), INS_IsAtomicUpdate(ins),
                                          operand_list, mcpat_instruction);

      INS_InsertCall(
         ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
         IARG_THREAD_ID,
         IARG_BOOL, TRUE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);
      INS_InsertCall(
         ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
         IARG_THREAD_ID,
         IARG_BOOL, FALSE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);
   }

   // Now handle instructions which have a static cost
   else
   {
      instruction = new Instruction(instruction_type, INS_Opcode(ins),
                                    INS_Address(ins), INS_Size(ins), INS_IsAtomicUpdate(ins),
                                    operand_list, mcpat_instruction);
   }

   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(handleInstruction),
                  IARG_THREAD_ID,
                  IARG_PTR, instruction,
                  IARG_END);
}
