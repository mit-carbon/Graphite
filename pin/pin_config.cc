#include "pin_config.h"
#include "simulator.h"

PinConfig *PinConfig::m_singleton = NULL;

void PinConfig::allocate()
{
   assert (m_singleton == NULL);
   m_singleton = new PinConfig();
}

void PinConfig::release()
{
   delete m_singleton;
   m_singleton = NULL;
}

PinConfig *PinConfig::getSingleton()
{
   return m_singleton;
}

PinConfig::PinConfig()
{
   m_current_process_num = Sim()->getConfig()->getCurrentProcessNum();
   m_total_cores = Sim()->getConfig()->getTotalCores();
   m_num_local_cores = Sim()->getConfig()->getNumLocalCores();
   
   setStackBoundaries();
}

PinConfig::~PinConfig()
{
}

// INIT function to set the stack limits
void PinConfig::setStackBoundaries()
{
   IntPtr global_stack_base = (IntPtr) (Sim()->getCfg()->getInt("stack/stack_base"));
   IntPtr global_stack_size = (IntPtr) (Sim()->getCfg()->getInt("stack/stack_size"));

   // It might be easier to just pass in 'm_stack_size_per_core' rather than 'global_stack_size'
   // We dont need a stack for the MCP
   m_stack_size_per_core = global_stack_size / (m_total_cores - 1);
   
   // To calculate our stack base, we need to get the total number of cores
   // allocated to processes that have ids' lower than me
   UInt32 num_cores = 0;
   for (SInt32 i = 0; i < (SInt32) m_current_process_num; i++)
   {
      num_cores += Sim()->getConfig()->getNumCoresInProcess(i);
   }
   
   m_stack_lower_limit = global_stack_base + num_cores * m_stack_size_per_core;
   m_stack_upper_limit = m_stack_lower_limit + m_num_local_cores * m_stack_size_per_core;
}

// Get Core ID from stack pointer
core_id_t PinConfig::getCoreIDFromStackPtr(IntPtr stack_ptr)
{
   if ( (stack_ptr < m_stack_lower_limit) || (stack_ptr > m_stack_upper_limit) )
   {
      return -1;
   }     
  
   SInt32 core_index = (SInt32) ((stack_ptr - m_stack_lower_limit) / m_stack_size_per_core);

   return (Config::getSingleton()->getCoreIDFromIndex(m_current_process_num, core_index));
}

SInt32 PinConfig::getStackAttributesFromCoreID (core_id_t core_id, StackAttributes& stack_attr)
{
   // Get the stack attributes
   SInt32 core_index = Config::getSingleton()->getIndexFromCoreID(m_current_process_num, core_id);
   LOG_ASSERT_ERROR (core_index != -1, "Core %i does not belong to Process %i", 
         core_id, Config::getSingleton()->getCurrentProcessNum());

   stack_attr.lower_limit = m_stack_lower_limit + (core_index * m_stack_size_per_core);
   stack_attr.size = m_stack_size_per_core;

   return 0;
}

