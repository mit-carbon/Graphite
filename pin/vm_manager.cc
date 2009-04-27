VMManager::VMManager()
{
   // FIXME: Figure out a way to get the end of the static data     
   m_start_data_segment = sbrk(0);
   m_end_data_segment = m_start_data_segment;

   m_start_stack_segment = (IntPtr) (Sim()->getCfg()->getInt("stack/stack_base"));
   m_end_stack_segment = m_start_stack_segment + (IntPtr) (Sim()->getCfg()->getInt("stack/stack_size"));

   assert(m_start_stack_segment > m_start_data_segment);

   m_start_dynamic_segment = 0xc0000000;
   m_end_dynamic_segment = m_start_dynamic_segment;

   assert(m_start_dynamic_segment > m_start_stack_segment);
}

void *VMManager::brk(void end_data_segment)
{
   if (end_data_segment == (void*) 0)
   {
      return m_end_data_segment;
   }

   assert((IntPtr) end_data_segment > m_start_data_segment);
   assert((IntPtr) end_data_segment < m_start_stack_segment);

   m_end_data_segment = end_data_segment;

   return ((void*) m_end_data_segment);
}

void *VMManager::mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset)
{
   assert(fd == -1);
   assert(flags & MAP_ANONYMOUS == MAP_ANONYMOUS);
   assert(flags & MAP_FIXED == 0);
   assert(flags & MAP_PRIVATE == MAP_PRIVATE);
   
   assert((m_start_dynamic_segment - length) > m_end_stack_segment);

   m_start_dynamic_segment -= length;

   return ((void*) m_start_dynamic_segment);
}

int *VMManager::munmap(void *start, size_t length)
{
   // Ignore for now
   assert((IntPtr) start >= m_start_dynamic_segment);
   return 0;
}
