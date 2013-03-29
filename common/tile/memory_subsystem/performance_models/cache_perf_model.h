#pragma once

#include "fixed_types.h"
#include "time_types.h"

class CachePerfModel
{
public:
   enum CacheAccess_t 
   {
      ACCESS_CACHE_DATA_AND_TAGS = 0,
      ACCESS_CACHE_DATA,
      ACCESS_CACHE_TAGS,
      NUM_CACHE_ACCESS_TYPES
   };

   enum PerfModel_t
   {
      CACHE_PERF_MODEL_PARALLEL = 0,
      CACHE_PERF_MODEL_SEQUENTIAL,
      NUM_CACHE_PERF_MODELS
   };

public:
   CachePerfModel(UInt64 data_array_access_time, UInt64 tag_array_access_time, float core_frequency);
   virtual ~CachePerfModel();

   static CachePerfModel* create(std::string cache_perf_model_type, UInt64 data_array_access_time,
                                 UInt64 tag_array_access_time, float core_frequency);
   static PerfModel_t parseModelType(std::string model_type);

   virtual void enable() = 0;
   virtual void disable() = 0;
   virtual bool isEnabled() = 0;

   virtual Time getLatency(CacheAccess_t access) = 0;

   void updateInternalVariablesOnFrequencyChange(float old_frequency, float new_frequency);

protected:
   UInt64 _data_array_access_cycles;
   UInt64 _tag_array_access_cycles;
   Time _data_array_access_time;
   Time _tag_array_access_time;
};
