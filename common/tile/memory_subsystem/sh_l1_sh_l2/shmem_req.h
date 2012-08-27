#pragma once

#include "shmem_msg.h"
#include "utils.h"
#include "fixed_types.h"

namespace ShL1ShL2
{
   class ShmemReq
   {
   public:
      ShmemReq(tile_id_t sender, ShmemMsg* shmem_msg, UInt64 time);
      ~ShmemReq();

      tile_id_t getSender() { return _sender; }
      ShmemMsg* getShmemMsg() { return _shmem_msg; }
      UInt64 getTime() { return _time; }
      
      void setTime(UInt64 time) { _time = time; }
      void updateTime(UInt64 time) { _time = max<UInt64>(_time, time); }
   
   private:
      tile_id_t _sender;
      ShmemMsg* _shmem_msg;
      UInt64 _time;
   };
}
