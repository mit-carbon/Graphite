#include "pin_config.h"

PinConfig::PinConfig()
{
   m_current_process_num = Sim()->getConfig()->getCurrentProcessNum();
   m_total_cores = Sim()->getConfig()->getTotalCores();
   m_num_local_cores = Sim()->getConfig()->getNumLocalCores();
   
   setStackBoundaries();
}

// INIT function to set the stack limits
void PinConfig::setStackBoundaries()
{
   UInt32 global_stack_base = (UInt32) Sim()->getCfg()->getInt("stack/stack_base");
   UInt32 global_stack_size = (UInt32) Sim()->getCfg->getInt("stack/stack_size");

   // It might be easier to just pass in 'm_stack_size_per_core' rather than 'global_stack_size'
   // We dont need a stack for the MCP
   m_stack_size_per_core = global_stack_size / (m_total_cores - 1);
   
   // To calculate our stack base, we need to get the total number of cores
   // allocated to processes that have ids' lower than me
   UInt32 num_cores = 0;
   for (SInt32 i = 0; i < m_current_process_num; i++)
   {
      num_cores += getNumCoresInProcess(i);
   }
   
   m_stack_base = global_stack_base + num_cores * m_stack_size_per_core;
   m_stack_limit = m_stack_base + m_num_local_cores * m_stack_size_per_core;
}

// Get Core ID from stack pointer
core_id_t PinConfig::getCoreIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_base) || (stack_ptr > m_stack_limit) )
   {
      return -1;
   }     
  
   SInt32 core_index = (SInt32) ((stack_ptr - m_stack_base) / m_stack_size_per_core);

   return (getCoreIdFromIndex(m_current_process_num, core_index));
}

SInt32 PinConfig::getStackAttributesFromCoreID (core_id_t core_id, StackAttributes& stack_attr)
{
   // Get the stack attributes
   SInt32 core_index = Config::getSingleton()->getCoreIdFromIndex(m_current_process_num, core_id);
   LOG_ASSERT_ERROR (core_index != -1, "Core %i does not belong to Process %i", 
         core_id, Config::getSingleton()->getCurrentProcessNum());

   stack_attr.base = m_stack_base + (core_index * m_stack_size_per_core);
   stack_attr.size = m_stack_size_per_core;
}

