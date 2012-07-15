#pragma once

#include <cassert>
#include <iostream>
using std::ostream;

#include "lock.h"

class ShmemPerfModel
{
public:
   enum ThreadType
   {
      _APP_THREAD = 0,
      _SIM_THREAD,
      NUM_THREAD_TYPES
   };

   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCycleCount(ThreadType thread_type, UInt64 count);

   void setCycleCount(UInt64 count)
   {
      setCycleCount(getThreadType(), count);
   }
   UInt64 getCycleCount();
   void incrCycleCount(UInt64 count);
   void updateCycleCount(UInt64 count);

   void incrTotalMemoryAccessLatency(UInt64 shmem_time);

   void updateInternalVariablesOnFrequencyChange(volatile float core_frequency);
   
   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }
   bool isEnabled()  { return _enabled;  }

   void outputSummary(ostream& out, volatile float core_frequency);

private:
   UInt64 _cycle_count[NUM_THREAD_TYPES];
   bool _enabled;
   Lock _shmem_perf_model_lock;

   UInt64 _num_memory_accesses;
   UInt64 _total_memory_access_latency_in_clock_cycles;
   UInt64 _total_memory_access_latency_in_ns;

   ThreadType getThreadType();
   void initializePerformanceCounters();
};
