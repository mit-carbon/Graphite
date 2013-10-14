#include <cstdlib>
#include <cassert>

#include "pin_memory_manager.h"

PinMemoryManager::PinMemoryManager(Core* core):
   m_core(core)
{
   // Allocate scratchpads (aligned at 4 * sizeof (void*) to correctly handle
   // memory access instructions that require addresses to be aligned such as
   // MOVDQA
   for (unsigned int i = 0; i < NUM_ACCESS_TYPES; i++)
   {
      __attribute__((unused)) int status = posix_memalign ((void**) &m_scratchpad[i], 4 * sizeof (void*), m_scratchpad_size);
      assert (status == 0);
   }
}

PinMemoryManager::~PinMemoryManager()
{}

carbon_reg_t 
PinMemoryManager::redirectMemOp (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, UInt32 op_num, bool is_read)
{
   assert (op_num < NUM_ACCESS_TYPES);
   char *scratchpad = m_scratchpad [op_num];

   if (is_read)
   {
      Core::mem_op_t mem_op_type;
      Core::lock_signal_t lock_signal;

      if (has_lock_prefix)
      {
         // When we have a LOCK prefix, we do an exclusive READ
         mem_op_type = Core::READ_EX;
         lock_signal = Core::LOCK;
      }
      else
      {
         // When we DO NOT have a LOCK prefix, we do a normal READ
         mem_op_type = Core::READ;
         lock_signal = Core::NONE;
      }
       
      m_core->accessMemory(lock_signal, mem_op_type, tgt_ea, scratchpad, size, true);

   }
   return (carbon_reg_t) scratchpad;
}

void 
PinMemoryManager::completeMemWrite (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, UInt32 op_num)
{
   char *scratchpad = m_scratchpad [op_num];

   Core::lock_signal_t lock_signal = (has_lock_prefix) ? Core::UNLOCK : Core::NONE;

   m_core->accessMemory (lock_signal, Core::WRITE, tgt_ea, scratchpad, size, true);
}

carbon_reg_t 
PinMemoryManager::redirectPushf ( IntPtr tgt_esp, IntPtr size )
{
   m_saved_esp = tgt_esp;
   return ((carbon_reg_t) m_scratchpad [0]) + size;
}

carbon_reg_t 
PinMemoryManager::completePushf ( IntPtr esp, IntPtr size )
{
   m_saved_esp -= size;
   completeMemWrite (false, (IntPtr) m_saved_esp, size, 0);
   return m_saved_esp;
}

carbon_reg_t 
PinMemoryManager::redirectPopf (IntPtr tgt_esp, IntPtr size)
{
   m_saved_esp = tgt_esp;
   return redirectMemOp (false, m_saved_esp, size, 0, true);
}

carbon_reg_t 
PinMemoryManager::completePopf (IntPtr esp, IntPtr size)
{
   return (m_saved_esp + size);
}
