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
         tile_id_t m_tile_id;

      public:
         ShmemReq();
         ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
         ~ShmemReq();

         ShmemMsg* getShmemMsg() { return m_shmem_msg; }
         UInt64 getTime() { return m_time; }
         tile_id_t getTileId() { return m_tile_id; }
        
         void setShmemMsg(ShmemMsg* shmem_msg) { m_shmem_msg = shmem_msg; } 
         void setTime(UInt64 time) { m_time = time; }
         void updateTime(UInt64 time)
         {
            if (time > m_time)
               m_time = time;
         }
         void setTileId(tile_id_t tile_id) { m_tile_id = tile_id; }
   };
}
