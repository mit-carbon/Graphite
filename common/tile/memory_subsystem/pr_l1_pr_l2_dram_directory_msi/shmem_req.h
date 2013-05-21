#pragma once

#include "shmem_msg.h"
#include "fixed_types.h"
#include "time_types.h"

namespace PrL1PrL2DramDirectoryMSI
{
   class ShmemReq
   {
   public:
      ShmemReq(ShmemMsg* shmem_msg, Time time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg() const { return _shmem_msg; }
      Time getTime() const        { return _time; }
      
      void setTime(Time time)     { _time = time; }
      void updateTime(Time time);

   private:
      ShmemMsg* _shmem_msg;
      Time _time;
   };
}
