#include "shmem_perf_model.h"
#include "log.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"

ShmemPerfModel::ShmemPerfModel()
   : _curr_time(0)
   , _enabled(false)
{}

ShmemPerfModel::~ShmemPerfModel()
{}

void 
ShmemPerfModel::setCurrTime(const Time& time)
{
   LOG_PRINT("setCurrTime: time(%llu ns)", time.toNanosec());
   _curr_time = time;
}

Time
ShmemPerfModel::getCurrTime()
{
   return _curr_time;
}

void
ShmemPerfModel::updateCurrTime(const Time& time)
{
   LOG_PRINT("updateCurrTime: time(%llu ns)", time.toNanosec());
   if (_curr_time < time)
      _curr_time = time;
}

void
ShmemPerfModel::incrCurrTime(const Time& time)
{
   LOG_PRINT("incrCurrTime: time(%llu ps)", time.getTime());
   if (_enabled)
   {
      LOG_PRINT("incrCurrTime: time(%llu ns)", time.toNanosec());
      _curr_time += time;
   }
}
