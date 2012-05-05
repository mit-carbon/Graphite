#pragma once

#include "cache_perf_model.h"

class CachePerfModelSequential : public CachePerfModel
{
public:
   CachePerfModelSequential(UInt64 data_array_access_time, UInt64 tag_array_access_time, float core_frequency)
      : CachePerfModel(data_array_access_time, tag_array_access_time, core_frequency)
      , _enabled(false)
   {}
   ~CachePerfModelSequential() {}

   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }
   bool isEnabled()  { return _enabled;  }

   UInt64 getLatency(CacheAccess_t access)
   {
      if (!_enabled)
         return 0;

      switch(access)
      {
      case ACCESS_CACHE_TAGS:
         return _tag_array_access_time;

      case ACCESS_CACHE_DATA:
         return _data_array_access_time;

      case ACCESS_CACHE_DATA_AND_TAGS:
         return _data_array_access_time + _tag_array_access_time;

      default:
         fprintf(stderr, "Unrecognized cache access type(%u)\n", access);
         return 0;
      }
   }

private:
   bool _enabled;
};
