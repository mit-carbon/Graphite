#pragma once

#include <vector>
using std::vector;
#include "shmem_msg.h"
#include "fixed_types.h"
#include "directory_state.h"
#include "time_types.h"

namespace PrL1PrL2DramDirectoryMOSI
{
   class ShmemReq
   {
   public:
      ShmemReq(ShmemMsg* shmem_msg, Time time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg() const
      { return _shmem_msg; }
      Time getSerializationTime() const
      { return _processing_start_time - _arrival_time; }
      Time getProcessingTime() const
      { return _processing_finish_time - _processing_start_time; }
      
      Time getArrivalTime() const
      { return _arrival_time; }
      Time getProcessingStartTime() const
      { return _processing_start_time; }
      Time getProcessingFinishTime() const
      { return _processing_finish_time; }
      Time getTime() const
      { return _processing_finish_time; }
    
      void updateProcessingStartTime(Time time);
      void updateProcessingFinishTime(Time time);

      DirectoryState::Type getInitialDState() const
      { return _initial_dstate; }
      void setInitialDState(DirectoryState::Type dstate)
      { _initial_dstate = dstate; }
      
      bool getInitialBroadcastMode() const
      { return _initial_broadcast_mode; }
      void setInitialBroadcastMode(bool in_broadcast_mode)
      { _initial_broadcast_mode = in_broadcast_mode; }

      tile_id_t getSharerTileId() const
      { return _sharer_tile_id; }
      void setSharerTileId(tile_id_t tile_id)
      { _sharer_tile_id = tile_id; }

      void setUpgradeReply()
      { _upgrade_reply = true; }
      bool isUpgradeReply() const
      { return _upgrade_reply; }
  
   private:
      ShmemMsg* _shmem_msg;
      
      Time _arrival_time;
      Time _processing_start_time;
      Time _processing_finish_time;
      
      DirectoryState::Type _initial_dstate;
      bool _initial_broadcast_mode;
      tile_id_t _sharer_tile_id;
      bool _upgrade_reply;
   };
}
