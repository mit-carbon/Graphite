#pragma once

#include <vector>
using std::vector;
#include "shmem_msg.h"
#include "fixed_types.h"

namespace PrL1ShL2MSI
{

class ShmemReq
{
public:
   ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
   ~ShmemReq();

   ShmemMsg* getShmemMsg() const
   { return _shmem_msg; }
   UInt64 getTime() const
   { return _time; }
   void updateTime(UInt64 time);

   void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);

private:
   ShmemMsg* _shmem_msg;
   UInt64 _time;
};

}
