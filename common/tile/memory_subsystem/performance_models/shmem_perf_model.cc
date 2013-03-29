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
ShmemPerfModel::setCurrTime(Time time)
{
   LOG_PRINT("setCurrTime: time(%llu ps)", time.getTime());
   _curr_time = time;
}

Time
ShmemPerfModel::getCurrTime()
{
   return _curr_time;
}

void
ShmemPerfModel::updateCurrTime(Time time)
{
   LOG_PRINT("updateCurrTime: time(%llu ps)", time.getTime());
   if (_curr_time < time)
      _curr_time = time;
}

void
ShmemPerfModel::incrCurrTime(Time time)
{
   if (_enabled)
      _curr_time += time;
}


