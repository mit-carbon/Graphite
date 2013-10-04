#include "shmem_req.h"
#include "utils.h"
#include "log.h"

namespace PrL1ShL2MSI
{

ShmemReq::ShmemReq(ShmemMsg* shmem_msg, Time time)
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

void
ShmemReq::updateTime(Time time)
{
   if (_time < time)
      _time = time;
}

}
