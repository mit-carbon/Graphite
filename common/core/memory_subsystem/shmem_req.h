#ifndef __SHMEM_REQ_H__
#define __SHMEM_REQ_H__

#include "shmem_msg.h"
#include "fixed_types.h"

class ShmemReq
{
   private:
      ShmemMsg* m_shmem_msg;
      UInt64 m_time;
      core_id_t m_responder_core_id;

   public:
      ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg() { return m_shmem_msg; }
      UInt64 getTime() { return m_time; }
      core_id_t getResponderCoreId() { return m_responder_core_id; }
      
      void setTime(UInt64 time) { m_time = time; }
      void updateTime(UInt64 time) 
      {
         if (time > m_time)
            m_time = time;
      }
      void setResponderCoreId(core_id_t core_id) { m_responder_core_id = core_id; }
};

#endif /* __SHMEM_REQ_H__ */
