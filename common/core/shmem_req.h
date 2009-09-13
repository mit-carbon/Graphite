#ifndef __SHMEM_REQ_H__
#define __SHMEM_REQ_H__

#include "shmem_msg.h"
#include "fixed_types.h"

class ShmemReq
{
   private:
      ShmemMsg* m_shmem_msg;
      core_id_t m_sender;
      UInt64 m_time;

   public:
      ShmemReq(ShmemMsg* shmem_msg, core_id_t sender, UInt64 time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg() { return m_shmem_msg; }
      core_id_t getSender() { return m_sender; }
      UInt64 getTime() { return m_time; }
};

#endif /* __SHMEM_REQ_H__ */
