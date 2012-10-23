#include "shmem_perf_model.h"
#include "log.h"

ShmemPerfModel::ShmemPerfModel()
   : _cycle_count(0)
   , _enabled(false)
{}

ShmemPerfModel::~ShmemPerfModel()
{}

void 
ShmemPerfModel::setCycleCount(UInt64 count)
{
   LOG_PRINT("setCycleCount: count(%llu)", count);
   _cycle_count = count;
}

UInt64
ShmemPerfModel::getCycleCount()
{
   return _cycle_count;
}

void
ShmemPerfModel::updateCycleCount(UInt64 cycle_count)
{
   LOG_PRINT("updateCycleCount: cycle_count(%llu)", cycle_count);
   if (_cycle_count < cycle_count)
      _cycle_count = cycle_count;
}

void
ShmemPerfModel::incrCycleCount(UInt64 count)
{
   if (_enabled)
      _cycle_count += count;
}
