#include "shmem_req.h"
#include "utils.h"
#include "clock_converter.h"
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
      _processing_finish_time = time;
}

void
ShmemReq::updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency)
{
   _arrival_time = convertCycleCount(_arrival_time, old_frequency, new_frequency);
   _processing_start_time = convertCycleCount(_processing_start_time, old_frequency, new_frequency);
   _processing_finish_time = convertCycleCount(_processing_finish_time, old_frequency, new_frequency);
}

}
