#include <cmath>

#include "clock_converter.h"
#include "log.h"

UInt64 convertCycleCount(UInt64 cycle_count, volatile float from_frequency, volatile float to_frequency)
{
   LOG_PRINT("convertCycleCount(): cycle_count(%llu), from_frequency(%f), to_frequency(%f)",
         cycle_count, from_frequency, to_frequency);

   return (UInt64) ceil(((double) cycle_count) * (to_frequency / from_frequency));
}
