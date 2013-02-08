#include "shmem_perf_model.h"
#include "log.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"

ShmemPerfModel::ShmemPerfModel()
   : _curr_time(0)
   //, _cycle_count(0)
   , _enabled(false)
{}


ShmemPerfModel::~ShmemPerfModel()
{}


void 
ShmemPerfModel::setCurrTime(Time& time)
{
   //LOG_PRINT("setCurrTime: count(%llu)", time._picosec);
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
   //LOG_PRINT("updateCurrTime: time(%llu)", time._picosec);
   if (_curr_time < time)
      _curr_time = time;
}

void
ShmemPerfModel::incrCurrTime(Latency& lat)
{
   if (_enabled)
      _curr_time = _curr_time + lat;
}




void 
ShmemPerfModel::setCycleCount(UInt64 count)
{
   LOG_PRINT("setCycleCount: count(%llu)", count);
   Latency lat(count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   Time time(lat.toPicosec());
   setCurrTime(time);

  // _cycle_count = count;
}

UInt64
ShmemPerfModel::getCycleCount()
{

//   LOG_PRINT("_curr_time(%llu), _cycle_count(%llu)", _curr_time.toCycles(Sim()->getTileManager()->getCurrentTile()->getFrequency()), _cycle_count);

 //  return _cycle_count;

   return _curr_time.toCycles(Sim()->getTileManager()->getCurrentTile()->getFrequency());
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   Latency lat(cycle_count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   Time time(lat.toPicosec());
   updateCurrTime(time);

   //if (_cycle_count < cycle_count)
    //  _cycle_count = cycle_count;
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   Latency lat(count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   //Time time(lat.toPicosec());
   incrCurrTime(lat);

   //if (_enabled)
   //   _cycle_count += count;
}


