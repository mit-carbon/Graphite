#pragma once

#include <vector>
using std::vector;
#include "shmem_msg.h"
#include "fixed_types.h"
#include "time_types.h"

namespace PrL1ShL2MSI
{

class ShmemReq
{
public:
   ShmemReq(ShmemMsg* shmem_msg, Time time);
   ~ShmemReq();

   ShmemMsg* getShmemMsg() const
   { return _shmem_msg; }
   Time getTime() const
   { return _time; }
   void updateTime(Time time);

private:
   ShmemMsg* _shmem_msg;
   Time _time;
};

}
