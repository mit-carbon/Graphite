#ifndef __CACHE_PERF_MODEL_H__
#define __CACHE_PERF_MODEL_H__

#include "fixed_types.h"

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

   protected:
      UInt32 m_cache_data_access_time;
      UInt32 m_cache_tags_access_time;

   public:
      CachePerfModel(UInt32 cache_data_access_time, UInt32 cache_tags_access_time);
      virtual ~CachePerfModel();

      static CachePerfModel* create(std::string cache_perf_model_type,
            UInt32 cache_data_access_time,
            UInt32 cache_tags_access_time);
      static PerfModel_t parseModelType(std::string model_type);

      virtual UInt32 getLatency(CacheAccess_t access) = 0;
};

#endif /* __CACHE_PERF_MODEL_H__ */
