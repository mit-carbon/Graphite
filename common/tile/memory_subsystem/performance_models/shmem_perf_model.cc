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
ShmemPerfModel::setCurrTime(Time& time)
{
   LOG_PRINT("setCurrTime: count(%llu)", time._picosec);
   _curr_time = time;
}

Time
ShmemPerfModel::getCurrTime()
{
   return _curr_time;
}

void
ShmemPerfModel::updateCurrTime(Time& time)
{
   LOG_PRINT("updateCurrTime: time(%llu)", time._picosec);
   if (_curr_time < time)
      _curr_time = time;
}

void
ShmemPerfModel::incrCurrTime(Time& time)
{
   if (_enabled)
      _curr_time = _curr_time + time;
}




void 
ShmemPerfModel::setCycleCount(UInt64 count)
{
   LOG_PRINT("setCycleCount: count(%llu)", count);
   Latency lat(count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   Time time(lat.toPicosec());
   setCurrTime(time);
}

UInt64
ShmemPerfModel::getCycleCount()
{

   return _curr_time.toCycles(Sim()->getTileManager()->getCurrentTile()->getFrequency());
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   LOG_PRINT("updateCycleCount: cycle_count(%llu)", cycle_count);
   Latency lat(cycle_count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   Time time(lat.toPicosec());
   updateCurrTime(time);
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   Latency lat(count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   Time time(lat.toPicosec());
   incrCurrTime(time);
}


