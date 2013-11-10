#include "instruction_modeling.h"
#include "simulator.h"
#include "core_model.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "core_model.h"
#include "hash_map.h"

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
   DynamicInstructionInfo info = DynamicInstructionInfo::createBranchInfo(taken, target);
   core_model->pushDynamicInstructionInfo(info);
}

void fillOperandListMemOps(OperandList *list, INS ins)
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
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
         
         else if (INS_OperandIsReg (ins, 0))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));

         else if (INS_OperandIsMemory (ins, 0))
         {
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
         }
      }
      
      else if (INS_Opcode (ins) == XED_ICLASS_POP)
      {
         if (INS_OperandIsReg (ins, 0))
         {
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
         }

         else if (INS_OperandIsMemory (ins, 0))
         {
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
         }
      }
     
      else if (INS_IsCall (ins))
      {
         if (INS_OperandIsMemory (ins, 0))
         {
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
         }

         else
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
      }

      else if (INS_IsRet (ins))
         list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

      else if (INS_Opcode (ins) == XED_ICLASS_LEAVE)
         list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

      else if ((INS_Opcode (ins) == XED_ICLASS_PUSHF) || (INS_Opcode (ins) == XED_ICLASS_PUSHFD))
         list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));

      else if ((INS_Opcode (ins) == XED_ICLASS_POPF) || (INS_Opcode (ins) == XED_ICLASS_POPFD))
         list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));
   
      // mem ops
      else if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
      {
         if (INS_IsMemoryRead (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

         if (INS_HasMemoryRead2 (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

         if (INS_IsMemoryWrite (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
      }
   }
   
   else // Sim()->getConfig()->getSimulationMode() == Config::LITE
   {
      // mem ops
      if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
      {
         if (INS_IsMemoryRead (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

         if (INS_HasMemoryRead2 (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::READ));

         if (INS_IsMemoryWrite (ins))
            list->push_back(Operand(Operand::MEMORY, 0, Operand::WRITE));
      }
   }
}

VOID fillOperandList(OperandList *list, INS ins)
{
   // memory
   fillOperandListMemOps(list, ins);

   // for handling register operands
   unsigned int max_read_regs = INS_MaxNumRRegs(ins);
   unsigned int max_write_regs = INS_MaxNumRRegs(ins);

   for (unsigned int i = 0; i < max_read_regs; i++)
   {
      if (REG_valid(INS_RegR(ins, i)))
         list->push_back(Operand(Operand::REG, INS_RegR(ins, i), Operand::READ));
   }

   for (unsigned int i = 0; i < max_write_regs; i++)
   {
      if (REG_valid(INS_RegW(ins, i)))
         list->push_back(Operand(Operand::REG, INS_RegW(ins, i), Operand::WRITE));
   }

   // immediate
   for (unsigned int i = 0; i < INS_OperandCount(ins); i++)
   {
      if (INS_OperandIsImmediate(ins, i))
      {
         list->push_back(Operand(Operand::IMMEDIATE, INS_OperandImmediate(ins, i), Operand::READ));
      }
   }
}

VOID addInstructionModeling(INS ins)
{
   Instruction* instruction;

   OperandList list;
   fillOperandList(&list, ins);

   // branches
   if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
   {
      instruction = new BranchInstruction(INS_Opcode(ins), list);

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
      OPCODE inst_opcode = INS_Opcode(ins);
      
      // FPU Opcode
      if
         (
          (inst_opcode == XED_ICLASS_FADD) || (inst_opcode == XED_ICLASS_FADDP) ||
          (inst_opcode == XED_ICLASS_FIADD) ||
          (inst_opcode == XED_ICLASS_FSUB) || (inst_opcode == XED_ICLASS_FSUBP) ||
          (inst_opcode == XED_ICLASS_FISUB) ||
          (inst_opcode == XED_ICLASS_FSUBR) || (inst_opcode == XED_ICLASS_FSUBRP) ||
          (inst_opcode == XED_ICLASS_FISUBR)
         )
      {
         instruction = new ArithInstruction(INST_FALU, INS_Opcode(ins), list);
      }
      else if 
         (
          (inst_opcode == XED_ICLASS_FMUL)||(inst_opcode == XED_ICLASS_FMULP)||
          (inst_opcode == XED_ICLASS_FIMUL)
         )
      {
         instruction = new ArithInstruction(INST_FMUL, INS_Opcode(ins), list);
      }
      else if 
         (
          (inst_opcode == XED_ICLASS_FDIV)||(inst_opcode == XED_ICLASS_FDIVP)||
          (inst_opcode == XED_ICLASS_FDIVR)||(inst_opcode == XED_ICLASS_FDIVRP)||
          (inst_opcode == XED_ICLASS_FIDIV)||(inst_opcode == XED_ICLASS_FIDIVR)
         )
      {
         instruction = new ArithInstruction(INST_FDIV, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_XMM_SS, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_XMM_SD, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_XMM_PS, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_IALU, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_IALU, INS_Opcode(ins), list);
      }
            
      // Bitwise operations
      else if
         (
          (inst_opcode == XED_ICLASS_OR) || (inst_opcode == XED_ICLASS_AND) ||
          (inst_opcode == XED_ICLASS_XOR) || (inst_opcode == XED_ICLASS_ANDNPS)
         )
      {
         instruction = new ArithInstruction(INST_IALU, INS_Opcode(ins), list);
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
         instruction = new ArithInstruction(INST_IALU, INS_Opcode(ins), list);
      }
            
      // Integer Multiplication 
      else if 
         (
          (inst_opcode == XED_ICLASS_MUL)||(inst_opcode == XED_ICLASS_IMUL)||
          (inst_opcode == XED_ICLASS_PCLMULQDQ)||(inst_opcode == XED_ICLASS_PFMUL)||
          (inst_opcode == XED_ICLASS_PMULDQ)||(inst_opcode == XED_ICLASS_PMULHRSW)||
          (inst_opcode == XED_ICLASS_PMULHRW)||(inst_opcode == XED_ICLASS_PMULHUW)||
          (inst_opcode == XED_ICLASS_PMULHW)||(inst_opcode == XED_ICLASS_PMULLD)||
          (inst_opcode == XED_ICLASS_PMULLW)||(inst_opcode == XED_ICLASS_PMULUDQ)
         )
      {
         // MUL Opcode
         instruction = new ArithInstruction(INST_IMUL, INS_Opcode(ins), list);
         //cout << "IMUL Opcode: " << INS_Mnemonic(ins) << " [" << INS_Opcode(ins) << "]" << endl;
      }

      // Integer Division
      else if 
         (
          (inst_opcode == XED_ICLASS_DIV) || (inst_opcode == XED_ICLASS_IDIV)
         )
      {
         // DIV Opcode
         instruction = new ArithInstruction(INST_IDIV, INS_Opcode(ins), list);
         //cout << "IDIV Opcode: " << INS_Mnemonic(ins) << " [" << INS_Opcode(ins) << "]" << endl;
      }

      // Generic Instruction with no functional unit
      else
      {
         // INT Opcode
         instruction = new ArithInstruction(INST_GENERIC, INS_Opcode(ins), list);
         //cout << "IALU Opcode: " << INS_Mnemonic(ins) << " [" << INS_Opcode(ins) << "]" << endl;
      }
   }

   instruction->setAddress(INS_Address(ins));
   instruction->setSize(INS_Size(ins));

   INS_InsertCall(ins, IPOINT_BEFORE,
                  AFUNPTR(handleInstruction),
                  IARG_THREAD_ID,
                  IARG_PTR, instruction,
                  IARG_END);
}
