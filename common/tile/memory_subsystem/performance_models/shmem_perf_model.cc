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
ShmemPerfModel::incrCurrTime(Latency lat)
{
   if (_enabled)
      _curr_time += lat;
}


void 
ShmemPerfModel::setCycleCount(UInt64 count)
{
   LOG_PRINT("setCycleCount: count(%llu)", count);
   Latency lat(count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   setCurrTime(Time(lat.toPicosec()));
}

UInt64
ShmemPerfModel::getCycleCount()
{
   return _curr_time.toCycles(Sim()->getTileManager()->getCurrentTile()->getFrequency());
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   Latency lat(cycle_count, Sim()->getTileManager()->getCurrentTile()->getFrequency());
   updateCurrTime(Time(lat.toPicosec()));
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   incrCurrTime(Latency(count, Sim()->getTileManager()->getCurrentTile()->getFrequency()));
}


