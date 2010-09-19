#ifndef __VM_MANAGER_H__
#define __VM_MANAGER_H__

#include <unistd.h>
#include <sys/mman.h>

#include "fixed_types.h"

class VMManager
{
   public:
      VMManager();
      ~VMManager();

      void *brk(void *end_data_segment);
      void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
      void *mmap2(void *start, size_t length, int prot, int flags, int fd, off_t offset);
      int munmap(void *start, size_t length);

   private:
      IntPtr m_start_data_segment;
      IntPtr m_end_data_segment;
      
      IntPtr m_start_stack_segment;
      IntPtr m_end_stack_segment;

      IntPtr m_start_dynamic_segment;
      IntPtr m_end_dynamic_segment;
};

#endif /* __VM_MANAGER_H__ */
