#ifndef __SHMEM_REQ_TYPES_H__
#define __SHMEM_REQ_TYPES_H__

enum shmem_req_t 
{
   READ = 0,
   READ_EX,
   WRITE,
   INVALIDATE,
   NUM_SHMEM_REQ_TYPES
};

#endif /* __SHMEM_REQ_TYPES_H__ */
