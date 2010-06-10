#include <cmath>

#include "clock_converter.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "network.h"
#include "network_model.h"
#include "fxsupport.h"
#include "log.h"

UInt64 convertCycleCount(ConversionType conversion_type, UInt64 cycle_count, void* ptr1, void* ptr2)
{
   UInt64 converted_cycle_count = UInt64(0);

   if (Fxsupport::isInitialized())
      Fxsupport::getSingleton()->fxsave();

   LOG_PRINT("convertCycleCount(): conversion_type(%u), cycle_count(%llu), ptr1(%p), ptr2(%p)",
         conversion_type, cycle_count, ptr1, ptr2);

   switch (conversion_type)
   {
      case CORE_CLOCK_TO_NETWORK_CLOCK:
         {
            volatile float core_frequency = static_cast<Core*>(ptr1)->getPerformanceModel()->getFrequency();
            volatile float network_frequency = static_cast<NetworkModel*>(ptr2)->getFrequency();

            converted_cycle_count = UInt64(ceil((float(cycle_count) / core_frequency) * network_frequency));
         }
         break;

      case CORE_CLOCK_TO_GLOBAL_CLOCK:
         {
            volatile float core_frequency = static_cast<Core*>(ptr1)->getPerformanceModel()->getFrequency();

            converted_cycle_count = UInt64(ceil(float(cycle_count) / core_frequency));
         }
         break;

      case NETWORK_CLOCK_TO_CORE_CLOCK:
         {
            volatile float core_frequency = static_cast<Core*>(ptr1)->getPerformanceModel()->getFrequency();
            volatile float network_frequency = static_cast<NetworkModel*>(ptr2)->getFrequency();

            converted_cycle_count = UInt64(ceil((float(cycle_count) / network_frequency) * core_frequency));
         }
         break;

      case NETWORK_CLOCK_TO_GLOBAL_CLOCK:
         {
            volatile float network_frequency = static_cast<NetworkModel*>(ptr1)->getFrequency();

            converted_cycle_count = UInt64(ceil(float(cycle_count) / network_frequency));
         }
         break;

      case GLOBAL_CLOCK_TO_CORE_CLOCK:
         {
            volatile float core_frequency = static_cast<Core*>(ptr1)->getPerformanceModel()->getFrequency();
 
            converted_cycle_count = UInt64(ceil(float(cycle_count) * core_frequency));
         }
         break;

      case GLOBAL_CLOCK_TO_NETWORK_CLOCK:
         {
            volatile float network_frequency = static_cast<NetworkModel*>(ptr1)->getFrequency();

            converted_cycle_count = UInt64(ceil(float(cycle_count) * network_frequency));
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized Conversion Type(%u)", conversion_type);
         break;
   }
  
   if (Fxsupport::isInitialized())
      Fxsupport::getSingleton()->fxrstor();
   
   return converted_cycle_count;
}
