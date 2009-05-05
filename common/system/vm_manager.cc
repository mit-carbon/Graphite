#include "assert.h"
#include "vm_manager.h"
#include "simulator.h"
#include <boost/lexical_cast.hpp>

VMManager *VMManager::m_singleton = NULL;

void VMManager::allocate()
{
   assert (m_singleton == NULL);
   m_singleton = new VMManager();
}

void VMManager::release()
{
   delete m_singleton;
   m_singleton = NULL;
}

VMManager *VMManager::getSingleton()
{
   return m_singleton;
}

VMManager::VMManager()
{
   // FIXME: Figure out a way to get the end of the static data     
   m_start_data_segment = (IntPtr) sbrk(0);
   m_end_data_segment = m_start_data_segment;

   UInt32 total_cores = Sim()->getConfig()->getTotalCores();
  
   UInt32 stack_size_per_core;
   try
   {
      m_start_stack_segment = (IntPtr) boost::lexical_cast <unsigned long int> (Sim()->getCfg()->get("stack/stack_base"));
      stack_size_per_core = (UInt32) boost::lexical_cast <unsigned long int> (Sim()->getCfg()->get("stack/stack_size_per_core"));
   }
   catch (boost::bad_lexical_cast &)
   {
      cerr << "Error reading stack parameters from the config file" << endl;
      exit (-1);
   }

   // FIXME: MCP does not have a stack. Do something about this
   m_end_stack_segment = m_start_stack_segment + total_cores * stack_size_per_core; 

   assert(m_start_stack_segment > m_start_data_segment);

   m_start_dynamic_segment = 0xb0000000;
   m_end_dynamic_segment = m_start_dynamic_segment;

   assert(m_start_dynamic_segment > m_start_stack_segment);
   assert(m_start_dynamic_segment > m_end_stack_segment);
}

VMManager::~VMManager()
{
}

void *VMManager::brk(void *end_data_segment)
{
   if (end_data_segment == (void*) 0)
   {
      return (void*) m_end_data_segment;
   }

   assert((IntPtr) end_data_segment > m_start_data_segment);
   assert((IntPtr) end_data_segment < m_start_stack_segment);

   m_end_data_segment = (IntPtr) end_data_segment;

   return ((void*) m_end_data_segment);
}

void *VMManager::mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
   assert(fd == -1);
   assert((flags & MAP_ANONYMOUS) == MAP_ANONYMOUS);
   assert((flags & MAP_FIXED) == 0);
   assert((flags & MAP_PRIVATE) == MAP_PRIVATE);
   
   assert((m_start_dynamic_segment - length) > m_end_stack_segment);

   m_start_dynamic_segment -= length;

   return ((void*) m_start_dynamic_segment);
}

void *VMManager::mmap2(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
   assert(fd == -1);
   assert((flags & MAP_ANONYMOUS) == MAP_ANONYMOUS);
   assert((flags & MAP_FIXED) == 0);
   assert((flags & MAP_PRIVATE) == MAP_PRIVATE);
   
   assert((m_start_dynamic_segment - (length * getpagesize())) > m_end_stack_segment);

   m_start_dynamic_segment -= (length * getpagesize());

   return ((void*) m_start_dynamic_segment);
}

int VMManager::munmap(void *start, size_t length)
{
   // Ignore for now
   assert((IntPtr) start >= m_start_dynamic_segment);
   return 0;
}
