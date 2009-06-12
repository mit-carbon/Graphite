#ifndef __CACHE_PERF_MODEL_SEQUENTIAL_H__ 
#define __CACHE_PERF_MODEL_SEQUENTIAL_H__

#include "cache_perf_model_base.h"
#include "log.h"

class CachePerfModelSequential : public CachePerfModelBase
{
   private:

   public:
      CachePerfModelSequential() : CachePerfModelBase() { }
      ~CachePerfModelSequential() { }

      UInt32 getLatency(CacheAccess_t access)
      {
         switch(access)
         {
            case ACCESS_CACHE_TAGS:
               return cache_tags_access_time;

            case ACCESS_CACHE_DATA:
               return cache_data_access_time;

            case ACCESS_CACHE_DATA_AND_TAGS:
               return cache_data_access_time + cache_tags_access_time;

            default:
               LOG_ASSERT_ERROR(false, "Unsupported Cache Access Type: %u", access);
               return 0;
         }

      }
 
};

#endif /* __CACHE_PERF_MODEL_SEQUENTIAL_H__ */
