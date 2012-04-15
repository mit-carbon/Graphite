#pragma once

#include <vector>
using std::vector;
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
      
      DirectoryState::Type _initial_dstate;
      bool _initial_broadcast_mode;
      tile_id_t _sharer_tile_id;
      bool _upgrade_reply;

      // Oldest sharer birth time
      vector<UInt64> _oldest_sharer_birth_time;
      // Cache capacity savings counters
      vector<AggregateCacheLineLifetime> _cache_capacity_savings_by_PCT;

      // Update birth time of oldest sharer
      void updateOldestSharerBirthTime(UInt32 private_copy_threshold, UInt64 birth_time);
      // Add back the lifetime of a single sharer
      void subtractRemoteL2Lifetime(UInt32 private_copy_threshold, UInt64 curr_time);

   public:
      ShmemReq(ShmemMsg* shmem_msg, UInt64 time);
      ~ShmemReq();

      ShmemMsg* getShmemMsg() const
      { return _shmem_msg; }
      UInt64 getSerializationTime() const
      { return _processing_start_time - _arrival_time; }
      UInt64 getProcessingTime() const
      { return _processing_finish_time - _processing_start_time; }
      UInt64 getTime() const
      { return _processing_finish_time; }
      UInt64 getArrivalTime() const
      { return _arrival_time; }
      UInt64 getProcessingStartTime() const
      { return _processing_start_time; }
      UInt64 getProcessingFinishTime() const
      { return _processing_finish_time; }
     
      void updateProcessingStartTime(UInt64 time);
      void updateProcessingFinishTime(UInt64 time);

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

      void getCacheCapacitySavings(UInt32 private_copy_threshold, UInt64 curr_time,
                                   AggregateCacheLineLifetime& cache_capacity_savings);
      void updateCacheCapacitySavingsCounters(UInt32 private_copy_threshold, UInt64 cache_line_birth_time,
                                              AggregateCacheLineLifetime aggregate_lifetime);
   };
}
