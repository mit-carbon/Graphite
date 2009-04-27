#ifndef __VM_MANAGER_H__
#define __VM_MANAGER_H__

class VMManager
{
public:
   void *brk(void *end_data_segment);
   void *mmap(void *start, size_t length, int prot, int flags, int fd, off_t offset);
   int munmap(void *start, size_t length);
private:
   VMManager();
   ~VMManager();

   IntPtr m_start_data_segment;
   IntPtr m_end_data_segment;

   IntPtr m_start_dynamic_segment;
   IntPtr m_end_dynamic_segment;
};


#endif /* __VM_MANAGER_H__ */
