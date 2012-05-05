#include "common_defines.h"
#include "shmem_req.h"
#include "utils.h"
#include "log.h"

namespace PrL1ShL2MSI
{

ShmemReq::ShmemReq(ShmemMsg* shmem_msg, UInt64 time)
   : _time(time)
{
   // Make a local copy of the shmem_msg
   _shmem_msg = new ShmemMsg(shmem_msg);
   LOG_ASSERT_ERROR(shmem_msg->getDataBuf() == NULL, "Shmem Reqs should not have data payloads");
}

ShmemReq::~ShmemReq()
{
   delete _shmem_msg;
}

}
