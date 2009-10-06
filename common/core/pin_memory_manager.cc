#include "pin_memory_manager.h"

PinMemoryManager::PinMemoryManager(Core* core):
   m_core(core)
{}

PinMemoryManager::~PinMemoryManager()
{}

carbon_reg_t 
PinMemoryManager::redirectMemOp (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type)
{
   assert (access_type < NUM_ACCESS_TYPES);
   char *scratchpad = m_scratchpad [access_type];
   
   if ((access_type == ACCESS_TYPE_READ) || (access_type == ACCESS_TYPE_READ2))
   {
      Core::mem_op_t mem_op_type;
      Core::lock_signal_t lock_signal;

      if (has_lock_prefix)
      {
         // FIXME: Now, when we have a LOCK prefix, we do an exclusive READ
         mem_op_type = Core::READ_EX;
         lock_signal = Core::LOCK;
      }
      else
      {
         mem_op_type = Core::READ;
         lock_signal = Core::NONE;
      }
       
      m_core->accessMemory (lock_signal, mem_op_type, tgt_ea, scratchpad, size, true);

   }
   return (carbon_reg_t) scratchpad;
}

void 
PinMemoryManager::completeMemWrite (bool has_lock_prefix, IntPtr tgt_ea, IntPtr size, AccessType access_type)
{
   char *scratchpad = m_scratchpad [access_type];

   Core::lock_signal_t lock_signal = (has_lock_prefix) ? Core::UNLOCK : Core::NONE;
      
   m_core->accessMemory (lock_signal, Core::WRITE, tgt_ea, scratchpad, size, true);
   
   return;
}

carbon_reg_t 
PinMemoryManager::redirectPushf ( IntPtr tgt_esp, IntPtr size )
{
   m_saved_esp = tgt_esp;
   return ((carbon_reg_t) m_scratchpad [ACCESS_TYPE_WRITE]) + size;
}

carbon_reg_t 
PinMemoryManager::completePushf ( IntPtr esp, IntPtr size )
{
   m_saved_esp -= size;
   completeMemWrite (false, (IntPtr) m_saved_esp, size, ACCESS_TYPE_WRITE);
   return m_saved_esp;
}

carbon_reg_t 
PinMemoryManager::redirectPopf (IntPtr tgt_esp, IntPtr size)
{
   m_saved_esp = tgt_esp;
   return redirectMemOp (false, m_saved_esp, size, ACCESS_TYPE_READ);
}

carbon_reg_t 
PinMemoryManager::completePopf (IntPtr esp, IntPtr size)
{
   return (m_saved_esp + size);
}
