#ifndef __CLOCK_CONVERTER_H__
#define __CLOCK_CONVERTER_H__

#include <stdlib.h>
#include "fixed_types.h"

enum ConversionType
{
   CORE_CLOCK_TO_NETWORK_CLOCK = 0,
   CORE_CLOCK_TO_GLOBAL_CLOCK,
   NETWORK_CLOCK_TO_CORE_CLOCK,
   NETWORK_CLOCK_TO_GLOBAL_CLOCK,
   GLOBAL_CLOCK_TO_CORE_CLOCK,
   GLOBAL_CLOCK_TO_NETWORK_CLOCK,
   NUM_CONVERSION_TYPES
};

UInt64 convertCycleCount(ConversionType conversion_type, UInt64 cycle_count, void* ptr1 = NULL, void* ptr2 = NULL);

#endif /* __CLOCK_CONVERTER_H__ */
