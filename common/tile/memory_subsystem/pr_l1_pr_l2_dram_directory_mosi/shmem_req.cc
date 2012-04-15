#include "common_defines.h"
#include "shmem_req.h"
#include "utils.h"
#include "log.h"

namespace PrL1PrL2DramDirectoryMOSI
{

ShmemReq::ShmemReq(ShmemMsg* shmem_msg, UInt64 time)
   : _arrival_time(time)
   , _processing_start_time(time)
   , _processing_finish_time(time)
   , _initial_dstate(DirectoryState::UNCACHED)
   , _initial_broadcast_mode(false)
   , _sharer_tile_id(INVALID_TILE_ID)
   , _upgrade_reply(false)
{
   // Make a local copy of the shmem_msg
   _shmem_msg = new ShmemMsg(shmem_msg);
   LOG_ASSERT_ERROR(shmem_msg->getDataBuf() == NULL, 
         "Shmem Reqs should not have data payloads");
   
   // Cache capacity savings counters
   _oldest_sharer_birth_time.resize(MAX_PRIVATE_COPY_THRESHOLD + 1, UINT64_MAX_);
   _cache_capacity_savings_by_PCT.resize(MAX_PRIVATE_COPY_THRESHOLD + 1);
}

ShmemReq::~ShmemReq()
{
   delete _shmem_msg;
}

void
ShmemReq::updateProcessingStartTime(UInt64 time)
{
   if (_processing_start_time < time)
   {
      _processing_start_time = time;
      _processing_finish_time = time;
   }
}

void
ShmemReq::updateProcessingFinishTime(UInt64 time)
{
   if (_processing_finish_time < time)
   {
      _processing_finish_time = time;
   }
}

void
ShmemReq::getCacheCapacitySavings(UInt32 private_copy_threshold, UInt64 curr_time,
                                  AggregateCacheLineLifetime& cache_capacity_savings)
{
   // Always consider one copy living in the L2 cache
   subtractRemoteL2Lifetime(private_copy_threshold, curr_time);
   cache_capacity_savings = _cache_capacity_savings_by_PCT[private_copy_threshold];
   // Reset the birth time of the oldest sharer to infinity
   _oldest_sharer_birth_time[private_copy_threshold] = UINT64_MAX_;
}

void
ShmemReq::updateCacheCapacitySavingsCounters(UInt32 private_copy_threshold, UInt64 birth_time,
                                             AggregateCacheLineLifetime aggregate_lifetime)
{
   // Update the birth time of the oldest sharer
   updateOldestSharerBirthTime(private_copy_threshold, birth_time);
   // Increment the counters that track the savings in L1 & L2 cache capacity
   _cache_capacity_savings_by_PCT[private_copy_threshold] += aggregate_lifetime;
}

void
ShmemReq::subtractRemoteL2Lifetime(UInt32 private_copy_threshold, UInt64 curr_time)
{
   // Take the minimum to prevent underflow
   UInt64 diff = (curr_time > _oldest_sharer_birth_time[private_copy_threshold]) ?
                 (curr_time - _oldest_sharer_birth_time[private_copy_threshold]) : 0;
   UInt64 remote_L2_lifetime = getMin<UInt64>(diff, _cache_capacity_savings_by_PCT[private_copy_threshold].L2);
   _cache_capacity_savings_by_PCT[private_copy_threshold].L2 -= remote_L2_lifetime;
}

void
ShmemReq::updateOldestSharerBirthTime(UInt32 private_copy_threshold, UInt64 birth_time)
{
   _oldest_sharer_birth_time[private_copy_threshold] = getMin<UInt64>(_oldest_sharer_birth_time[private_copy_threshold],
                                                                      birth_time);
}

}
