#pragma once

#include "shmem_msg.h"
#include "fixed_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class ShmemReq
   {
      private:
         ShmemMsg* m_shmem_msg;
         UInt64 m_time;
         core_id_t m_core_id;

      public:
         ShmemReq();
         ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
         ~ShmemReq();

         ShmemMsg* getShmemMsg() { return m_shmem_msg; }
         UInt64 getTime() { return m_time; }
         core_id_t getCoreId() { return m_core_id; }
        
         void setShmemMsg(ShmemMsg* shmem_msg) { m_shmem_msg = shmem_msg; } 
         void setTime(UInt64 time) { m_time = time; }
         void updateTime(UInt64 time)
         {
            if (time > m_time)
               m_time = time;
         }
         void setCoreId(core_id_t core_id) { m_core_id = core_id; }
   };
}
