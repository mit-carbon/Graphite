#include "instruction_modeling.h"

#include "simulator.h"
#include "core_model.h"
#include "opcodes.h"
#include "tile_manager.h"
#include "tile.h"

void handleBasicBlock(BasicBlock *sim_basic_block)
{
   CoreModel *prfmdl = Sim()->getTileManager()->getCurrentCore()->getPerformanceModel();

   prfmdl->queueBasicBlock(sim_basic_block);

   //FIXME: put this in a thread
   prfmdl->iterate();
}

void handleBranch(BOOL taken, ADDRINT target)
{
   assert(Sim() && Sim()->getTileManager() && Sim()->getTileManager()->getCurrentTile());
   CoreModel *prfmdl = Sim()->getTileManager()->getCurrentCore()->getPerformanceModel();

   DynamicInstructionInfo info = DynamicInstructionInfo::createBranchInfo(taken, target);
   prfmdl->pushDynamicInstructionInfo(info);
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

   BasicBlock *basic_block = new BasicBlock();

   OperandList list;
   fillOperandList(&list, ins);

   // branches
   if (INS_IsBranch(ins) && INS_HasFallThrough(ins))
   {
      basic_block->push_back(new BranchInstruction(list));

      INS_InsertCall(
         ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)handleBranch,
         IARG_BOOL, TRUE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);

      INS_InsertCall(
         ins, IPOINT_AFTER, (AFUNPTR)handleBranch,
         IARG_BOOL, FALSE,
         IARG_BRANCH_TARGET_ADDR,
         IARG_END);
   }

   // Now handle instructions which have a static cost
   else
   {
      switch(INS_Opcode(ins))
      {
      case OPCODE_DIV:
         basic_block->push_back(new ArithInstruction(INST_DIV, list));
         break;
      case OPCODE_MUL:
         basic_block->push_back(new ArithInstruction(INST_MUL, list));
         break;
      case OPCODE_FDIV:
         basic_block->push_back(new ArithInstruction(INST_FDIV, list));
         break;
      case OPCODE_FMUL:
         basic_block->push_back(new ArithInstruction(INST_FMUL, list));
         break;

      default:
         basic_block->push_back(new GenericInstruction(list));
         break;
      }
   }

   basic_block->front()->setAddress(INS_Address(ins));
   basic_block->front()->setSize(INS_Size(ins));

   INS_InsertCall(ins, IPOINT_BEFORE, AFUNPTR(handleBasicBlock), IARG_PTR, basic_block, IARG_END);
}
