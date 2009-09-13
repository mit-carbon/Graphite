#include "shmem_req.h"
#include "log.h"

ShmemReq::ShmemReq(ShmemMsg* shmem_msg, core_id_t sender, UInt64 time):
   m_sender(sender),
   m_time(time)
{
   // Make a local copy of the shmem_msg
   m_shmem_msg = new ShmemMsg(shmem_msg);
   LOG_ASSERT_ERROR(shmem_msg->m_data_buf == NULL, 
         "Shmem Reqs should not have data payloads");
}

ShmemReq::~ShmemReq()
{
   delete m_shmem_msg;
}
