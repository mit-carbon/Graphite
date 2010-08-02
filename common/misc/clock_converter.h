#ifndef __CLOCK_CONVERTER_H__
#define __CLOCK_CONVERTER_H__

#include "fixed_types.h"

UInt64 convertCycleCount(UInt64 cycle_count, volatile float from_frequency, volatile float to_frequency);

#endif /* __CLOCK_CONVERTER_H__ */
