#pragma once

#include <iostream>
using std::ostream;

class ShmemPerfModel
{
public:
   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCycleCount(UInt64 count);
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
   UInt64 _cycle_count;
   bool _enabled;

   UInt64 _num_memory_accesses;
   UInt64 _total_memory_access_latency_in_clock_cycles;
   UInt64 _total_memory_access_latency_in_ns;

   void initializePerformanceCounters();
};
