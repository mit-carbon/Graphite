#include "redirect_memory.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "memory_manager.h"
#include "performance_modeler.h"

// FIXME
// Only need this function because some memory accesses are made before cores have
// been initialized. Should not evnentually need this

VOID printInsInfo(CONTEXT* ctxt)
{
   ADDRINT reg_inst_ptr = PIN_GetContextReg(ctxt, REG_INST_PTR);
   ADDRINT reg_stack_ptr = PIN_GetContextReg(ctxt, REG_STACK_PTR);

   LOG_PRINT("eip = 0x%x, esp = 0x%x\n", reg_inst_ptr, reg_stack_ptr);
}

UINT32 memOp (Core::lock_signal_t lock_signal, shmem_req_t shmem_req_type, IntPtr d_addr, char *data_buffer, UInt32 data_size)
{
   assert (lock_signal == Core::NONE);

   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      return core->accessMemory (lock_signal, shmem_req_type, d_addr, data_buffer, data_size);
   }
   // Native mem op
   else
   {
      if (shmem_req_type == READ)
      {
         if (PIN_SafeCopy ((void*) data_buffer, (void*) d_addr, (size_t) data_size) == data_size)
         {
            return 0;
         }
      }
      else if (shmem_req_type == WRITE)
      {
         if (PIN_SafeCopy ((void*) d_addr, (void*) data_buffer, (size_t) data_size) == data_size)
         {
            return 0;
         }
      }
      else
      {
         assert (false);
      }

      return 0;
   }
}

bool rewriteStringOp (INS ins)
{
   if (INS_Opcode(ins) == XED_ICLASS_SCASB)
   {
      LOG_PRINT("Instr: %s\n", (INS_Disassemble(ins)).c_str());
      if (! (INS_RepPrefix(ins) || INS_RepnePrefix(ins)) )
      {
         return false;
      }

      INS_InsertCall(ins, IPOINT_BEFORE,
            AFUNPTR (emuSCASBIns),
            IARG_CONTEXT,
            IARG_ADDRINT, INS_NextAddress(ins),
            IARG_BOOL, INS_RepPrefix(ins),
            IARG_END);

      INS_Delete(ins);

      return true;
   }

   else if (INS_Opcode(ins) == XED_ICLASS_CMPSB)
   {
      LOG_PRINT("Instr: %s\n", (INS_Disassemble(ins)).c_str());
      if (! (INS_RepPrefix(ins) || INS_RepnePrefix(ins)) )
      {
         return false;
      }

      INS_InsertCall(ins, IPOINT_BEFORE,
            AFUNPTR (emuCMPSBIns),
            IARG_CONTEXT,
            IARG_ADDRINT, INS_NextAddress(ins),
            IARG_BOOL, INS_RepPrefix(ins),
            IARG_END);

      INS_Delete(ins);
      
      return true;
   }

   else
   {
      assert(INS_Opcode(ins) != XED_ICLASS_SCASW);
      assert(INS_Opcode(ins) != XED_ICLASS_SCASD);
      assert(INS_Opcode(ins) != XED_ICLASS_SCASQ);

      assert(INS_Opcode(ins) != XED_ICLASS_CMPSW);
      assert(INS_Opcode(ins) != XED_ICLASS_CMPSD);
      assert(INS_Opcode(ins) != XED_ICLASS_CMPSQ);

      return false;
   }
}


void emuCMPSBIns(CONTEXT *ctxt, ADDRINT next_gip, bool has_rep_prefix)
{
   ADDRINT reg_gip = PIN_GetContextReg(ctxt, REG_INST_PTR);
   LOG_PRINT("Instr at EIP = 0x%x, CMPSB\n", reg_gip);

   assert(has_rep_prefix == true);
   
   ADDRINT reg_gcx = PIN_GetContextReg(ctxt, REG_GCX);
   ADDRINT reg_gsi = PIN_GetContextReg(ctxt, REG_GSI);
   ADDRINT reg_gdi = PIN_GetContextReg(ctxt, REG_GDI);
   ADDRINT reg_gflags = PIN_GetContextReg(ctxt, REG_GFLAGS);

   bool direction_flag;

   // Direction Flag
   if( (reg_gflags & (1 << 10)) == 0 )
   {
      // Forward
      direction_flag = false;
   }
   else
   {
      // Backward
      direction_flag = true;
   }

   bool found = false;

   while (reg_gcx > 0)
   {
      Byte byte_buf1;
      Byte byte_buf2;

      memOp (Core::NONE, READ, reg_gsi, (char*) &byte_buf1, sizeof(byte_buf1));
      memOp (Core::NONE, READ, reg_gdi, (char*) &byte_buf2, sizeof(byte_buf2));

      // Decrement the counter
      reg_gcx --;
      // Increment/Decrement EDI to show that we are moving forward
      if (!direction_flag) 
      {
         reg_gsi ++;
         reg_gdi ++;
      }
      else
      {
         reg_gsi --;
         reg_gdi --;
      }

      if (byte_buf1 != byte_buf2)
      {
         found = true;
         break;
      }
   }

   if (! found)
   {
      // Set the 'zero' flag
      reg_gflags |= (1 << 6);
      // reg_gflags |= "Whatever is Zero Flag";
   }
   else
   {
      // Clear the 'zero' flag
      reg_gflags &= (~(1 << 6));
   }

   PIN_SetContextReg(ctxt, REG_INST_PTR, next_gip);
   PIN_SetContextReg(ctxt, REG_GCX, reg_gcx);
   PIN_SetContextReg(ctxt, REG_GSI, reg_gsi);
   PIN_SetContextReg(ctxt, REG_GDI, reg_gdi);
   PIN_SetContextReg(ctxt, REG_GFLAGS, reg_gflags);

   PIN_ExecuteAt(ctxt);

}

void emuSCASBIns(CONTEXT *ctxt, ADDRINT next_gip, bool has_rep_prefix)
{
   ADDRINT reg_gip = PIN_GetContextReg(ctxt, REG_INST_PTR);
   LOG_PRINT("Instr at EIP = 0x%x, SCASB\n", reg_gip);

   assert(has_rep_prefix == false);
   
   ADDRINT reg_gcx = PIN_GetContextReg(ctxt, REG_GCX);
   ADDRINT reg_gdi = PIN_GetContextReg(ctxt, REG_GDI);
   ADDRINT reg_gax = PIN_GetContextReg(ctxt, REG_GAX);
   ADDRINT reg_gflags = PIN_GetContextReg(ctxt, REG_GFLAGS);

   LOG_PRINT("reg_gcx = 0x%x\n", reg_gcx);
   LOG_PRINT("reg_gdi = 0x%x\n", reg_gdi);
   LOG_PRINT("reg_gax = 0x%x\n", reg_gax);
   LOG_PRINT("reg_gflags = 0x%x\n", reg_gflags);

   Byte reg_al = (Byte) (reg_gax & 0xff);
   LOG_PRINT("reg_al = 0x%x\n", reg_al);

   bool direction_flag;

   // Direction Flag
   if( (reg_gflags & (1 << 10)) == 0 )
   {
      // Forward
      direction_flag = false;
   }
   else
   {
      // Backward
      direction_flag = true;
   }

   LOG_PRINT("Direction Flag = %i\n", (SInt32) direction_flag);

   bool found = false;

   while (reg_gcx > 0)
   {
      Byte byte_buf;
      memOp (Core::NONE, READ, reg_gdi, (char*) &byte_buf, sizeof(byte_buf));

      LOG_PRINT("byte_buf = 0x%x\n", byte_buf);

      // Decrement the counter
      reg_gcx --;
      // Increment/Decrement EDI to show that we are moving forward
      if (!direction_flag)
      {
         reg_gdi ++;
      }
      else
      {
         reg_gdi --;
      }

      if (byte_buf == reg_al)
      {
         found = true;
         break;
      }
   }

   if (found)
   {
      // Set the 'zero' flag
      reg_gflags |= (1 << 6);
      // reg_gflags |= "Whatever is Zero Flag";
   }
   else
   {
      // Clear the 'zero' flag
      reg_gflags &= (~(1 << 6));
   }

   LOG_PRINT("Final: next_gip = 0x%x\n", next_gip);
   LOG_PRINT("Final: reg_gcx = 0x%x\n", reg_gcx);
   LOG_PRINT("Final: reg_gdi = 0x%x\n", reg_gdi);
   LOG_PRINT("Final: reg_gflags = 0x%x\n", reg_gflags);

   PIN_SetContextReg(ctxt, REG_INST_PTR, next_gip);
   PIN_SetContextReg(ctxt, REG_GCX, reg_gcx);
   PIN_SetContextReg(ctxt, REG_GDI, reg_gdi);
   PIN_SetContextReg(ctxt, REG_GFLAGS, reg_gflags);

   PIN_ExecuteAt(ctxt);

}

bool rewriteStackOp (INS ins)
{
   if (INS_Opcode (ins) == XED_ICLASS_PUSH)
   {
      if (INS_OperandIsImmediate (ins, 0))
      {
         ADDRINT value = INS_OperandImmediate (ins, 0);
         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (emuPushValue),
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
            IARG_REG_REFERENCE, REG_STACK_PTR,
            IARG_UINT32, imm,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_INST_G1,
            IARG_END);

      INS_InsertIndirectJump (ins, IPOINT_AFTER, REG_INST_G1);

      INS_Delete (ins);
      return true;
   }

   else if (INS_Opcode (ins) == XED_ICLASS_LEAVE)
   {
      INS_InsertCall (ins, IPOINT_BEFORE, AFUNPTR (emuLeave),
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
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYWRITE_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      INS_InsertCall (ins, IPOINT_AFTER,
            AFUNPTR (completePushf),
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
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      INS_InsertCall (ins, IPOINT_AFTER,
            AFUNPTR (completePopf),
            IARG_REG_VALUE, REG_STACK_PTR,
            IARG_MEMORYREAD_SIZE,
            IARG_RETURN_REGS, REG_STACK_PTR,
            IARG_END);

      return true;
   }

   return false;
}

void rewriteMemOp (INS ins)
{
   if (INS_IsMemoryRead (ins) || INS_IsMemoryWrite (ins))
   {
      if (INS_RewriteMemoryAddressingToBaseRegisterOnly (ins, MEMORY_TYPE_READ, REG_INST_G0))
      {
         INS_InsertCall (ins, IPOINT_BEFORE, 
               AFUNPTR (redirectMemOp),
               IARG_BOOL, INS_IsAtomicUpdate(ins),
               IARG_MEMORYREAD_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_UINT32, MemoryManager::ACCESS_TYPE_READ ,
               IARG_RETURN_REGS, REG_INST_G0,
               IARG_END);
      }
      else if (INS_IsMemoryRead (ins))
      {
         LOG_PRINT_ERROR ("Could not rewrite memory read at ip = 0x%x", INS_Address(ins));
      }

      if (INS_RewriteMemoryAddressingToBaseRegisterOnly (ins, MEMORY_TYPE_READ2, REG_INST_G1))
      {
         assert(! INS_IsAtomicUpdate(ins));

         INS_InsertCall (ins, IPOINT_BEFORE, 
               AFUNPTR (redirectMemOp),
               IARG_BOOL, false,
               IARG_MEMORYREAD2_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_UINT32, MemoryManager::ACCESS_TYPE_READ2,
               IARG_RETURN_REGS, REG_INST_G1,
               IARG_END);
      }
      else if (INS_HasMemoryRead2 (ins))
      {
         LOG_PRINT_ERROR ("Could not rewrite memory read2 at ip = 0x%x", INS_Address(ins));
      }

      if (INS_RewriteMemoryAddressingToBaseRegisterOnly (ins, MEMORY_TYPE_WRITE, REG_INST_G2))
      {
         assert(! INS_IsAtomicUpdate(ins));

         INS_InsertCall (ins, IPOINT_BEFORE,
               AFUNPTR (redirectMemOp),
               IARG_BOOL, false,
               IARG_MEMORYWRITE_EA,
               IARG_MEMORYWRITE_SIZE,
               IARG_UINT32, MemoryManager::ACCESS_TYPE_WRITE,
               IARG_RETURN_REGS, REG_INST_G2,
               IARG_END);

         IPOINT ipoint = INS_HasFallThrough (ins) ? IPOINT_AFTER : IPOINT_TAKEN_BRANCH;
         assert (ipoint == IPOINT_AFTER);

         INS_InsertCall (ins, ipoint, 
               AFUNPTR (completeMemWrite),
               IARG_BOOL, false,
               IARG_MEMORYWRITE_EA,
               IARG_MEMORYWRITE_SIZE,
               IARG_UINT32, MemoryManager::ACCESS_TYPE_WRITE,
               IARG_END);
      }
      else if (INS_IsMemoryWrite (ins))
      {
         if (INS_IsMemoryRead (ins))
         {
            MemoryManager::AccessType access_type = MemoryManager::ACCESS_TYPE_WRITE;
            IPOINT ipoint2 = INS_HasFallThrough (ins) ? IPOINT_AFTER : IPOINT_TAKEN_BRANCH;
            assert (ipoint2 == IPOINT_AFTER);
            
            UINT32 operand_count = INS_OperandCount (ins);
            UINT32 num_mem_read_operands = 0;

            for (UINT32 i = 0; i < operand_count; i++)
            {
               if (INS_OperandIsMemory (ins, i) && INS_OperandReadAndWriten (ins, i))
               {
                  if (num_mem_read_operands == 0)
                  {
                     access_type = MemoryManager::ACCESS_TYPE_READ;
                  }
                  else if (num_mem_read_operands == 1)
                  {
                     access_type = MemoryManager::ACCESS_TYPE_READ2;
                  }
                  else
                  {
                     LOG_PRINT_ERROR ("Could not rewrite memory write at ip = 0x%x", INS_Address(ins));
                  }
                  num_mem_read_operands++;
               }
               else if (INS_OperandIsMemory (ins, i) && INS_OperandRead (ins, i))
               {
                  num_mem_read_operands++;
               }
            }

            assert (num_mem_read_operands > 0);
            assert (access_type != MemoryManager::ACCESS_TYPE_WRITE);

            INS_InsertCall (ins, ipoint2,
                  AFUNPTR (completeMemWrite),
                  IARG_BOOL, INS_IsAtomicUpdate(ins),
                  IARG_MEMORYWRITE_EA,
                  IARG_MEMORYWRITE_SIZE,
                  IARG_UINT32, access_type,
                  IARG_END);
         }
         else
         {
            LOG_PRINT_ERROR ("Could not rewrite memory write at ip = 0x%x", INS_Address(ins));
         }
      }
   }
}

ADDRINT emuPushValue (ADDRINT tgt_esp, ADDRINT value, ADDRINT write_size)
{
   assert (write_size != 0);
   assert ( write_size == sizeof ( ADDRINT ) );

   tgt_esp -= write_size;

   memOp (Core::NONE, WRITE, (IntPtr) tgt_esp, (char*) &value, (UInt32) write_size);
   
   return tgt_esp;
}

ADDRINT emuPushMem(ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size)
{
   assert (size != 0);
   assert ( size == sizeof ( ADDRINT ) );

   tgt_esp -= sizeof(ADDRINT);

   ADDRINT buf;

   memOp (Core::NONE, READ, (IntPtr) operand_ea, (char*) &buf, (UInt32) size);
   memOp (Core::NONE, WRITE, (IntPtr) tgt_esp, (char*) &buf, (UInt32) size);

   return tgt_esp;
}

ADDRINT emuPopReg(ADDRINT tgt_esp, ADDRINT *reg, ADDRINT read_size)
{
   assert (read_size != 0);
   assert ( read_size == sizeof ( ADDRINT ) );

   memOp (Core::NONE, READ, (IntPtr) tgt_esp, (char*) reg, (UInt32) read_size);
   
   return tgt_esp + read_size;
}

ADDRINT emuPopMem(ADDRINT tgt_esp, ADDRINT operand_ea, ADDRINT size)
{
   assert ( size == sizeof ( ADDRINT ) );
   
   ADDRINT buf;

   memOp (Core::NONE, READ, (IntPtr) tgt_esp, (char*) &buf, (UInt32) size);
   memOp (Core::NONE, WRITE, (IntPtr) operand_ea, (char*) &buf, (UInt32) size);

   return tgt_esp + size;
}

ADDRINT emuCallMem(ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT operand_ea, ADDRINT read_size, ADDRINT write_size)
{
   assert (read_size == sizeof(ADDRINT));
   assert (write_size == sizeof(ADDRINT));
   
   ADDRINT called_ip;
   memOp (Core::NONE, READ, (IntPtr) operand_ea, (char*) &called_ip, (UInt32) read_size);

   *tgt_esp = *tgt_esp - sizeof(ADDRINT);
   memOp (Core::NONE, WRITE, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) write_size);
   
   return called_ip;
}

ADDRINT emuCallRegOrImm(ADDRINT *tgt_esp, ADDRINT *tgt_eax, ADDRINT next_ip, ADDRINT br_tgt_ip, ADDRINT write_size)
{
   assert (write_size == sizeof(ADDRINT));
   
   *tgt_esp = *tgt_esp - sizeof(ADDRINT);

   memOp (Core::NONE, WRITE, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) write_size);
   
   return br_tgt_ip;
}

ADDRINT emuRet(ADDRINT *tgt_esp, UINT32 imm, ADDRINT read_size)
{
   assert ( read_size == sizeof ( ADDRINT ) );

   ADDRINT next_ip;

   memOp (Core::NONE, READ, (IntPtr) *tgt_esp, (char*) &next_ip, (UInt32) read_size);

   *tgt_esp = *tgt_esp + read_size;
   *tgt_esp = *tgt_esp + imm;
   
   return next_ip;
}

ADDRINT emuLeave(ADDRINT tgt_esp, ADDRINT *tgt_ebp, ADDRINT read_size)
{
   assert ( read_size == sizeof ( ADDRINT ) );

   tgt_esp = *tgt_ebp;

   memOp (Core::NONE, READ, (IntPtr) tgt_esp, (char*) tgt_ebp, (UInt32) read_size);
   
   tgt_esp += read_size;
   
   return tgt_esp;
}

ADDRINT redirectPushf ( ADDRINT tgt_esp, ADDRINT size )
{
   assert (size == sizeof (ADDRINT));

   Core *core = Sim()->getCoreManager()->getCurrentCore();
   
   if (core)
   {
      return core->getMemoryManager()->redirectPushf (tgt_esp, size);
   }
   else
   {
      return tgt_esp;
   }
}

ADDRINT completePushf ( ADDRINT esp, ADDRINT size )
{
   assert (size == sizeof(ADDRINT));
   
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
   {
      return core->getMemoryManager()->completePushf (esp, size);
   }
   else
   {
      return esp;
   }
}

ADDRINT redirectPopf (ADDRINT tgt_esp, ADDRINT size)
{
   assert (size == sizeof (ADDRINT));

   Core *core = Sim()->getCoreManager()->getCurrentCore();
  
   if (core)
   {
      return core->getMemoryManager()->redirectPopf (tgt_esp, size);
   }
   else
   {
      return tgt_esp;
   }
}

ADDRINT completePopf (ADDRINT esp, ADDRINT size)
{
   assert (size == sizeof (ADDRINT));
   
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
   {
      return core->getMemoryManager()->completePopf (esp, size);
   }
   else
   {
      return esp;
   }
}

// FIXME: 
// Memory accesses with a LOCK prefix made by cores are not handled correctly at the moment
// Once the memory accesses go through the coherent shared memory system, all LOCK'ed
// memory accesses from the cores would be handled correctly. 

ADDRINT redirectMemOp (bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, MemoryManager::AccessType access_type)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
  
   if (core)
   {
      MemoryManager *mem_manager = core->getMemoryManager ();
      assert (mem_manager != NULL);

      return (ADDRINT) mem_manager->redirectMemOp (has_lock_prefix, (IntPtr) tgt_ea, (IntPtr) size, access_type);
   }
   else
   {
      // Make sure that no instructions with the 
      // LOCK prefix execute in a non-core
      // assert (!has_lock_prefix);
      // cerr << "ins with LOCK prefix in a non-core" << endl;

      return tgt_ea;
   }
}

VOID completeMemWrite (bool has_lock_prefix, ADDRINT tgt_ea, ADDRINT size, MemoryManager::AccessType access_type)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();

   if (core)
   {
      core->getMemoryManager()->completeMemWrite (has_lock_prefix, (IntPtr) tgt_ea, (IntPtr) size, access_type);
   }
   else
   {
      // Make sure that no instructions with the 
      // LOCK prefix execute in a non-core
      // assert (!has_lock_prefix);
      // cerr << "ins with LOCK prefix in a non-core" << endl;
   }

   return;
}
