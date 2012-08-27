#include "shmem_req.h"
#include "log.h"

namespace ShL1ShL2
{
   ShmemReq::ShmemReq(tile_id_t sender, ShmemMsg* shmem_msg, UInt64 time)
      : _sender(sender)
      , _time(time)
   {
      // Make a local copy of the shmem_msg
      _shmem_msg = shmem_msg->clone();
   }

   ShmemReq::~ShmemReq()
   {
      _shmem_msg->release();
   }
}
