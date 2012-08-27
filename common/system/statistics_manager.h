#pragma once

#include <string>
using std::string;
#include "fixed_types.h"

class StatisticsManager
{
public:
   enum StatisticType
   {
      CACHE_LINE_REPLICATION = 0,
      NETWORK_UTILIZATION,
      NUM_STATISTIC_TYPES
   };

   StatisticsManager();
   ~StatisticsManager();
   void outputPeriodicSummary();
   UInt64 getSamplingInterval() { return _sampling_interval; }

private:
   bool _statistic_enabled[NUM_STATISTIC_TYPES];
   UInt64 _sampling_interval;

   void openTraceFiles();
   void closeTraceFiles();
   StatisticType parseType(string type);
};
