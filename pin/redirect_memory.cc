#include "redirect_memory.h"
#include "simulator.h"
#include "tile_manager.h"
#include "core.h"
#include "pin_memory_manager.h"
#include "core_model.h"
#include "hash_map.h"

extern HashMap core_map;

void memOp(THREADID thread_id, Core::lock_signal_t lock_signal, Core::mem_op_t mem_op_type, IntPtr d_addr, char *data_buffer, UInt32 data_size, BOOL push_info)
{   
   assert (lock_signal == Core::NONE);
   assert(thread_id != INVALID_THREADID);
   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   core->accessMemory(lock_signal, mem_op_type, d_addr, data_buffer, data_size, push_info);
}

bool rewriteStackOp(INS ins)
{
   if (INS_Opcode (ins) == XED_ICLASS_PUSH)
   {
      if (INS_OperandIsImmediate (ins, 0))
      {
         ADDRINT value = INS_OperandImmediate (ins, 0);
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuPushValue),
               IARG_THREAD_ID,
               IARG_REG_VALUE, REG_STACK_PTR,
               IARG_ADDRINT, value,
               IARG_MEMORYWRITE_SIZE,
               IARG_RETURN_REGS, REG_STACK_PTR,
               IARG_END);

         INS_Delete (ins);
         return true;
      }
      
      else if (INS_OperandIsReg (ins, 0))
      {
         REG reg = INS_OperandReg (ins, 0);
         INS_InsertCall (ins, IPOINT_BEFORE, 
               AFUNPTR (emuPushValue),
               IARG_THREAD_ID,
               IARG_REG_VALUE, REG_STACK_PTR,
               IARG_REG_VALUE, reg,
               IARG_MEMORYWRITE_SIZE,
               IARG_RETURN_REGS, REG_STACK_PTR,
               IARG_END);

         INS_Delete (ins);
         return true;
      }

      else if (INS_OperandIsMemory (ins, 0))
      {
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuPushMem),
               IARG_THREAD_ID,
               IARG_REG_VALUE, REG_STACK_PTR,
               IARG_MEMORYREAD_EA,
               IARG_MEMORYWRITE_SIZE,
               IARG_RETURN_REGS, REG_STACK_PTR,
               IARG_END);

         INS_Delete (ins);
         return true;
      }
   }
   
   else if (INS_Opcode (ins) == XED_ICLASS_POP)
   {
      if (INS_OperandIsReg (ins, 0))
      {
         REG reg = INS_OperandReg (ins, 0);
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuPopReg),
               IARG_THREAD_ID,
               IARG_REG_VALUE, REG_STACK_PTR,
               IARG_REG_REFERENCE, reg,
               IARG_MEMORYREAD_SIZE,
               IARG_RETURN_REGS, REG_STACK_PTR,
               IARG_END);

         INS_Delete (ins);
         return true;
      }

      else if (INS_OperandIsMemory (ins, 0))
      {
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuPopMem),
               IARG_THREAD_ID,
               IARG_MEMORYWRITE_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_RETURN_REGS, REG_STACK_PTR,
               IARG_END);

         INS_Delete (ins);
         return true;
      }
   }
  
   else if (INS_IsCall (ins))
   {
      ADDRINT next_ip = INS_NextAddress (ins);
      
      if (INS_OperandIsMemory (ins, 0))
      {
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuCallMem),
               IARG_THREAD_ID,
               IARG_REG_REFERENCE, REG_STACK_PTR,
               IARG_REG_REFERENCE, REG_GAX,
               IARG_ADDRINT, next_ip,
               IARG_MEMORYREAD_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_MEMORYWRITE_SIZE,
               IARG_RETURN_REGS, REG_INST_G0,
               IARG_END);
      }

      else
      {
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuCallRegOrImm),
               IARG_THREAD_ID,
               IARG_REG_REFERENCE, REG_STACK_PTR,
               IARG_REG_REFERENCE, REG_GAX,
               IARG_ADDRINT, next_ip,
               IARG_BRANCH_TARGET_ADDR,
               IARG_MEMORYWRITE_SIZE,
               IARG_RETURN_REGS, REG_INST_G0,
               IARG_END);
      }

      INS_InsertIndirectJump (ins, IPOINT_AFTER, REG_INST_G0);

      INS_Delete (ins);
      return true;
   }

   else if (INS_IsRet (ins))
   {
      UINT32 imm = 0;
      if ((INS_OperandCount (ins) > 0) && (INS_OperandIsImmediate (ins, 0)))
      {
         imm = INS_OperandImmediate (ins, 0);
      }

      INS_InsertCall (ins, IPOINT_BEFORE,
            AFUNPTR (emuRet),
            IARG_THREAD_ID,
            IARG_REG_REFERENCE, REG_STACK_PTR,
            IARG_UINT32, imm,
            IARG_MEMORYREAD_SIZE,
            IARG_BOOL, true,
            IARG_RETURN_REGS, REG_INST_G1,
            IARG_END);

      INS_InsertIndirectJump (ins, IPOINT_AFTER, REG_INST_G1);

      INS_Delete (ins);
      return true;
   }

   else if (INS_Opcode (ins) == XED_ICLASS_LEAVE)
   {
      INS_InsertCall (ins, IPOINT_BEFORE,
            AFUNPTR (emuLeave),
            IARG_THREAD_ID,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_REG_REFERENCE, REG_GBP,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      INS_Delete (ins);
      return true;
   }

   else if ((INS_Opcode (ins) == XED_ICLASS_PUSHF) || (INS_Opcode (ins) == XED_ICLASS_PUSHFD))
   {
      INS_InsertCall (ins, IPOINT_BEFORE, 
            AFUNPTR (redirectPushf),
            IARG_THREAD_ID,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYWRITE_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      INS_InsertCall (ins, IPOINT_AFTER,
            AFUNPTR (completePushf),
            IARG_THREAD_ID,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYWRITE_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      return true;
   }

   else if ((INS_Opcode (ins) == XED_ICLASS_POPF) || (INS_Opcode (ins) == XED_ICLASS_POPFD))
   {
      INS_InsertCall (ins, IPOINT_BEFORE,
            AFUNPTR (redirectPopf),
            IARG_THREAD_ID,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      INS_InsertCall (ins, IPOINT_AFTER,
            AFUNPTR (completePopf),
            IARG_THREAD_ID,
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      return true;
   }

   return false;
}

void rewriteMemOp(INS ins)
{
   LOG_ASSERT_ERROR(INS_MemoryOperandCount(ins) <= 3, 
                    "EIP(%#lx): Num Memory Operands(%u) > 3", INS_Address(ins), INS_MemoryOperandCount(ins));

   for (unsigned int i = 0; i < INS_MemoryOperandCount(ins); i++)
   {
      INS_InsertCall (ins, IPOINT_BEFORE, 
            AFUNPTR (redirectMemOp),
            IARG_THREAD_ID,
            IARG_BOOL, INS_IsAtomicUpdate(ins),
            IARG_MEMORYOP_EA, i,
            IARG_MEMORYREAD_SIZE,
            IARG_UINT32, i,
            IARG_BOOL, INS_MemoryOperandIsRead(ins, i),
            IARG_RETURN_REGS, REG(REG_INST_G0+i),
            IARG_END);
      
      INS_RewriteMemoryOperand(ins, i, REG(REG_INST_G0+i));

      if (INS_MemoryOperandIsWritten(ins, i))
      {
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (redirectMemOpSaveEa),
               IARG_MEMORYOP_EA, i,
               IARG_RETURN_REGS, REG_INST_G3,
               IARG_END);


         IPOINT ipoint = INS_HasFallThrough (ins) ? IPOINT_AFTER : IPOINT_TAKEN_BRANCH;
         assert (ipoint == IPOINT_AFTER);

         INS_InsertCall (ins, ipoint,
               AFUNPTR (completeMemWrite),
               IARG_THREAD_ID,
               IARG_BOOL, INS_IsAtomicUpdate(ins),
               IARG_REG_VALUE, REG_INST_G3, // Is IARG_MEMORYWRITE_EA,
               IARG_MEMORYWRITE_SIZE,
               IARG_UINT32, i,
               IARG_END);
      }
   }
}

ADDRINT emuPushValue(THREADID thread_id, ADDRINT tgt_esp, ADDRINT value, ADDRINT write_size)
{
   assert (write_size != 0);
   assert ( write_size == sizeof ( ADDRINT ) );

   tgt_esp -= write_size;

   memOp(thread_id, Core::NONE, Core::WRITE, (IntPtr) tgt_esp, (char*) &value, (UInt32) write_size);
   
   return tgt_esp;
}

ADDRINT emuPushMem(THREADID thread_id, ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size)
{
   assert (size != 0);
   assert (size == sizeof ( ADDRINT ));

   tgt_esp -= sizeof(ADDRINT);

   ADDRINT buf;

   memOp(thread_id, Core::NONE, Core::READ, (IntPtr) operand_ea, (char*) &buf, (UInt32) size);
   memOp(thread_id, Core::NONE, Core::WRITE, (IntPtr) tgt_esp, (char*) &buf, (UInt32) size);

   return tgt_esp;
}

ADDRINT emuPopReg(THREADID thread_id, ADDRINT tgt_esp, ADDRINT *reg, ADDRINT read_size)
{
   assert (read_size != 0);
   assert (read_size == sizeof ( ADDRINT ));

   memOp (thread_id, Core::NONE, Core::READ, (IntPtr) tgt_esp, (char*) reg, (UInt32) read_size);
   
   return tgt_esp + read_size;
}

ADDRINT emuPopMem(THREADID thread_id, ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size)
{
   assert ( size == sizeof ( ADDRINT ) );
   
   ADDRINT buf;

   memOp (thread_id, Core::NONE, Core::READ, (IntPtr) tgt_esp, (char*) &buf, (UInt32) size);
   memOp (thread_id, Core::NONE, Core::WRITE, (IntPtr) operand_ea, (char*) &buf, (UInt32) size);

   return tgt_esp + size;
}

ADDRINT emuCallMem(THREADID thread_id, ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT operand_ea, ADDRINT read_size, ADDRINT write_size)
{
   assert (read_size == sizeof(ADDRINT));
   assert (write_size == sizeof(ADDRINT));
   
   ADDRINT called_ip;
   memOp (thread_id, Core::NONE, Core::READ, (IntPtr) operand_ea, (char*) &called_ip, (UInt32) read_size);

   *tgt_esp = *tgt_esp - sizeof(ADDRINT);
   memOp (thread_id, Core::NONE, Core::WRITE, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) write_size);
   
   return called_ip;
}

ADDRINT emuCallRegOrImm(THREADID thread_id, ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT br_tgt_ip, ADDRINT write_size)
{
   assert (write_size == sizeof(ADDRINT));
   
   *tgt_esp = *tgt_esp - sizeof(ADDRINT);

   memOp (thread_id, Core::NONE, Core::WRITE, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) write_size);
   
   return br_tgt_ip;
}

ADDRINT emuRet(THREADID thread_id, ADDRINT *tgt_esp, UINT32 imm, ADDRINT read_size, BOOL push_info)
{
   assert ( read_size == sizeof ( ADDRINT ) );

   ADDRINT next_ip;

   memOp (thread_id, Core::NONE, Core::READ, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) read_size, push_info);

   *tgt_esp = *tgt_esp + read_size;
   *tgt_esp = *tgt_esp + imm;
   
   return next_ip;
}

ADDRINT emuLeave(THREADID thread_id, ADDRINT tgt_esp, ADDRINT *tgt_ebp, ADDRINT read_size)
{
   assert ( read_size == sizeof ( ADDRINT ) );

   tgt_esp = *tgt_ebp;

   memOp (thread_id, Core::NONE, Core::READ, (IntPtr) tgt_esp, (char*) tgt_ebp, (UInt32) read_size);
   
   tgt_esp += read_size;
   
   return tgt_esp;
}

ADDRINT redirectPushf(THREADID thread_id, ADDRINT tgt_esp, ADDRINT size)
{
   assert (size == sizeof (ADDRINT));

   Core *core = core_map.get<Core>(thread_id);
   assert(core); 
   return core->getPinMemoryManager()->redirectPushf(tgt_esp, size);
}

ADDRINT completePushf(THREADID thread_id, ADDRINT esp, ADDRINT size)
{
   assert (size == sizeof(ADDRINT));
   
   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   return core->getPinMemoryManager()->completePushf(esp, size);
}

ADDRINT redirectPopf(THREADID thread_id, ADDRINT tgt_esp, ADDRINT size)
{
   assert (size == sizeof (ADDRINT));

   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   return core->getPinMemoryManager()->redirectPopf(tgt_esp, size);
}

ADDRINT completePopf(THREADID thread_id, ADDRINT esp, ADDRINT size)
{
   assert (size == sizeof (ADDRINT));
   
   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   return core->getPinMemoryManager()->completePopf(esp, size);
}

ADDRINT redirectMemOp(THREADID thread_id, bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, UInt32 op_num, bool is_read)
{
   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   PinMemoryManager *mem_manager = core->getPinMemoryManager();
   return (ADDRINT) mem_manager->redirectMemOp(has_lock_prefix, (IntPtr) tgt_ea, (IntPtr) size, op_num, is_read);
}

ADDRINT redirectMemOpSaveEa(ADDRINT ea)
{
   return ea;
}

VOID completeMemWrite(THREADID thread_id, bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, UInt32 op_num)
{
   Core *core = core_map.get<Core>(thread_id);
   assert(core);
   core->getPinMemoryManager()->completeMemWrite (has_lock_prefix, (IntPtr) tgt_ea, (IntPtr) size, op_num);
}
