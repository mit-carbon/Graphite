#include <cmath>

#include "fixed_types.h"
#include "shmem_perf_model.h"
#include "simulator.h"
#include "tile_manager.h"
#include "fxsupport.h"

ShmemPerfModel::ShmemPerfModel()
   : _cycle_count(0)
   , _enabled(false)
{
   initializePerformanceCounters();
}

ShmemPerfModel::~ShmemPerfModel()
{}

void
ShmemPerfModel::initializePerformanceCounters()
{
   _num_memory_accesses = 0;
   _total_memory_access_latency_in_clock_cycles = 0;
   _total_memory_access_latency_in_ns = 0;
}

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

void
ShmemPerfModel::incrTotalMemoryAccessLatency(UInt64 memory_access_latency)
{
   if (_enabled)
   {
      _num_memory_accesses ++;
      _total_memory_access_latency_in_clock_cycles += memory_access_latency;
   }
}

void
ShmemPerfModel::updateInternalVariablesOnFrequencyChange(volatile float core_frequency)
{
   _total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(_total_memory_access_latency_in_clock_cycles) / core_frequency));
   _total_memory_access_latency_in_clock_cycles = 0;
}

void
ShmemPerfModel::outputSummary(ostream& out, volatile float core_frequency)
{
   _total_memory_access_latency_in_ns +=
      static_cast<UInt64>(ceil(static_cast<float>(_total_memory_access_latency_in_clock_cycles) / core_frequency));
   
   out << "Shared Memory Model summary: " << endl;
   out << "    Total Memory Accesses: " << _num_memory_accesses << endl;
   out << "    Average Memory Access Latency (in ns): " << 
      static_cast<float>(_total_memory_access_latency_in_ns) / _num_memory_accesses << endl;
}
