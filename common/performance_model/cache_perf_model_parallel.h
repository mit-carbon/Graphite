#ifndef __CACHE_PERF_MODEL_PARALLEL_H__ 
#define __CACHE_PERF_MODEL_PARALLEL_H__

#include "cache_perf_model_base.h"
#include "log.h"

class CachePerfModelParallel : public CachePerfModelBase
{
   private:

   public:
      CachePerfModelParallel () : CachePerfModelBase() { }
      ~CachePerfModelParallel() { }

      UInt32 getLatency(CacheAccess_t access)
      {
         switch(access)
         {
            case ACCESS_CACHE_TAGS:
               return cache_tags_access_time;

            case ACCESS_CACHE_DATA:
            case ACCESS_CACHE_DATA_AND_TAGS:
               return cache_data_access_time;

            default:
               LOG_ASSERT_ERROR(false, "Unsupported Cache Access Type: %u", access);
               return 0;
         }
      }
 
};

#endif /* __CACHE_PERF_MODEL_PARALLEL_H__ */
