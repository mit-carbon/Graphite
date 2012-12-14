#include <cmath>

#include "clock_converter.h"
#include "log.h"

UInt64 convertCycleCount(UInt64 cycle_count, volatile float from_frequency, volatile float to_frequency)
{
   UInt64 converted_cycle_count = (UInt64) ceil(((double) cycle_count) * (to_frequency / from_frequency));

   LOG_PRINT("convertCycleCount(%llu): from_frequency(%f), to_frequency(%f) -> (%llu)",
             cycle_count, from_frequency, to_frequency, converted_cycle_count);

   return converted_cycle_count;
}
