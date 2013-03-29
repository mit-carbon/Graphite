#pragma once

#include "cache_perf_model.h"
#include "log.h"

class CachePerfModelParallel : public CachePerfModel
{
public:
   CachePerfModelParallel(UInt64 data_array_access_cycles, UInt64 tag_array_access_cycles, float core_frequency)
      : CachePerfModel(data_array_access_cycles, tag_array_access_cycles, core_frequency)
      , _enabled(false)
   {}
   ~CachePerfModelParallel() {}

   void enable()     { _enabled = true;   }
   void disable()    { _enabled = false;  }
   bool isEnabled()  { return _enabled;   }

   Time getLatency(CacheAccess_t access)
   {
      if (!_enabled)
         return Time(0);

      switch(access)
      {
      case ACCESS_CACHE_TAGS:
         return _tag_array_access_time;

      case ACCESS_CACHE_DATA:
      case ACCESS_CACHE_DATA_AND_TAGS:
         return _data_array_access_time;

      default:
         LOG_PRINT_ERROR("Unrecognized cache access type(%u)", access);
         return Time(0);
      }
   }

private:
   bool _enabled;
};
