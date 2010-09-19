#include "shmem_req.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   ShmemReq::ShmemReq():
      m_time(0),
      m_core_id(INVALID_CORE_ID)
   {
      m_shmem_msg = new ShmemMsg();
   }

   ShmemReq::ShmemReq(ShmemMsg* shmem_msg, UInt64 time):
      m_time(time),
      m_core_id(INVALID_CORE_ID)
   {
      // Make a local copy of the shmem_msg
      m_shmem_msg = new ShmemMsg(shmem_msg);
      LOG_ASSERT_ERROR(shmem_msg->getDataBuf() == NULL, 
            "Shmem Reqs should not have data payloads");
   }

   ShmemReq::~ShmemReq()
   {
      delete m_shmem_msg;
   }
}
