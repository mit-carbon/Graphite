#ifndef __CACHE_PERF_MODEL_PARALLEL_H__ 
#define __CACHE_PERF_MODEL_PARALLEL_H__

#include "cache_perf_model.h"

class CachePerfModelParallel : public CachePerfModel
{
   public:
      CachePerfModelParallel(UInt32 cache_data_access_time, 
            UInt32 cache_tags_access_time) : 
         CachePerfModel(cache_data_access_time, cache_tags_access_time) 
      {}
      ~CachePerfModelParallel() {}

      UInt32 getLatency(CacheAccess_t access)
      {
         switch(access)
         {
            case ACCESS_CACHE_TAGS:
               return m_cache_tags_access_time;

            case ACCESS_CACHE_DATA:
            case ACCESS_CACHE_DATA_AND_TAGS:
               return m_cache_data_access_time;

            default:
               return 0;
         }
      }
 
};

#endif /* __CACHE_PERF_MODEL_PARALLEL_H__ */
