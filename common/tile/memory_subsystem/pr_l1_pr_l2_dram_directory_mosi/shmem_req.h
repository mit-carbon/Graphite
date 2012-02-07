#pragma once

#include "shmem_msg.h"
#include "fixed_types.h"
#include "directory_state.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class ShmemReq
   {
   private:
      ShmemMsg* _shmem_msg;
      
      UInt64 _arrival_time;
      UInt64 _processing_start_time;
      UInt64 _processing_finish_time;
      UInt64 _curr_time;
      
      DirectoryState::dstate_t _initial_dstate;
      bool _initial_broadcast_mode;
      tile_id_t _sharer_tile_id;

   public:
      ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg()             { return _shmem_msg; }
      UInt64 getSerializationTime()       { return _processing_start_time - _arrival_time; }
      UInt64 getProcessingTime()          { return _processing_finish_time - _processing_start_time; }
      UInt64 getTime()                    { return _curr_time; }
     
      void updateProcessingStartTime(UInt64 time);
      void updateProcessingFinishTime(UInt64 time);
      void updateTime(UInt64 time);

      DirectoryState::dstate_t getInitialDState()
      { return _initial_dstate; }
      void setInitialDState(DirectoryState::dstate_t dstate)
      { _initial_dstate = dstate; }
      
      bool getInitialBroadcastMode()
      { return _initial_broadcast_mode; }
      void setInitialBroadcastMode(bool in_broadcast_mode)
      { _initial_broadcast_mode = in_broadcast_mode; }

      tile_id_t getSharerTileId()
      { return _sharer_tile_id; }
      void setSharerTileId(tile_id_t tile_id)
      { _sharer_tile_id = tile_id; }
   };
}
