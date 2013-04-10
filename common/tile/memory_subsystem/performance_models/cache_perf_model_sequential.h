#pragma once

#include "cache_perf_model.h"
#include "log.h"

class CachePerfModelSequential : public CachePerfModel
{
public:
   CachePerfModelSequential(UInt64 data_access_cycles, UInt64 tags_access_cycles, float frequency)
      : CachePerfModel(data_access_cycles, tags_access_cycles, frequency)
   {}
   ~CachePerfModelSequential() {}

   Time getLatency(AccessType access_type)
   {
      switch (access_type)
      {
      case ACCESS_TAGS:
         return _tags_access_latency;

      case ACCESS_DATA:
         return _data_access_latency;

      case ACCESS_DATA_AND_TAGS:
         return _data_access_latency + _tags_access_latency;

      default:
         LOG_PRINT_ERROR("Unrecognized cache access type(%u)", access_type);
         return Time(0);
      }
   }
};
