#include "lite/memory_modeling.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "core.h"
#include "hash_map.h"

extern HashMap core_map;

namespace lite
{

void addMemoryModeling(INS ins)
{
   if (INS_IsMemoryRead(ins) || INS_IsMemoryWrite(ins))
   {
      if (INS_IsMemoryRead(ins))
      {
         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(handleMemoryRead),
               IARG_THREAD_ID,
               IARG_BOOL, INS_IsAtomicUpdate(ins),
               IARG_MEMORYREAD_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_END);
      }
      if (INS_HasMemoryRead2(ins))
      {
         LOG_ASSERT_ERROR(!INS_IsAtomicUpdate(ins), "Atomic Instruction has 2 read operands");

         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(handleMemoryRead),
               IARG_THREAD_ID,
               IARG_BOOL, false,
               IARG_MEMORYREAD2_EA,
               IARG_MEMORYREAD_SIZE,
               IARG_END);
      }
      if (INS_IsMemoryWrite(ins))
      {
         INS_InsertCall(ins, IPOINT_BEFORE,
               AFUNPTR(captureWriteEa),
               IARG_MEMORYWRITE_EA,
               IARG_RETURN_REGS, REG_INST_G0, /* store IARG_MEMORYWRITE_EA in G0 */
               IARG_END);

         IPOINT ipoint = INS_HasFallThrough(ins) ? IPOINT_AFTER : IPOINT_TAKEN_BRANCH;
         INS_InsertCall(ins, ipoint,
               AFUNPTR(handleMemoryWrite),
               IARG_THREAD_ID,
               IARG_BOOL, INS_IsAtomicUpdate(ins),
               IARG_REG_VALUE, REG_INST_G0, /* value of IARG_MEMORYWRITE_EA at IPOINT_BEFORE */
               IARG_MEMORYWRITE_SIZE,
               IARG_END);
      }
   }
}

void handleMemoryRead(THREADID thread_id, bool is_atomic_update, IntPtr read_address, UInt32 read_data_size)
{
   if (!Sim()->isEnabled())
      return;

   Byte read_data_buf[read_data_size];

   Core* core = core_map.get<Core>(thread_id);
   core->initiateMemoryAccess(MemComponent::L1_DCACHE,
         (is_atomic_update) ? Core::LOCK : Core::NONE,
         (is_atomic_update) ? Core::READ_EX : Core::READ,
         read_address,
         read_data_buf,
         read_data_size,
         true);
}

void handleMemoryWrite(THREADID thread_id, bool is_atomic_update, IntPtr write_address, UInt32 write_data_size)
{
   if (!Sim()->isEnabled())
      return;

   Core* core = core_map.get<Core>(thread_id);
   core->initiateMemoryAccess(MemComponent::L1_DCACHE,
         (is_atomic_update) ? Core::UNLOCK : Core::NONE,
         Core::WRITE,
         write_address,
         (Byte*) write_address,
         write_data_size,
         true);
}

IntPtr captureWriteEa(IntPtr tgt_ea)
{
   return tgt_ea;
}

}
