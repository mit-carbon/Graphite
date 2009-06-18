#ifndef __CACHE_PERF_MODEL_BASE_H__
#define __CACHE_PERF_MODEL_BASE_H__

#include "config.h"
#include "log.h"

#include "simulator.h"

class CachePerfModelBase
{
   public:
      enum CacheAccess_t 
      {
         ACCESS_CACHE_DATA_AND_TAGS = 0,
         ACCESS_CACHE_DATA,
         ACCESS_CACHE_TAGS,
         NUM_CACHE_ACCESS_TYPES
      };

      enum CacheModel_t
      {
         CACHE_MODEL_PARALLEL = 0,
         CACHE_MODEL_SEQUENTIAL,
         NUM_CACHE_MODELS
      };

   protected:
      UInt32 cache_data_access_time;
      UInt32 cache_tags_access_time;


   public:
      CachePerfModelBase()
      {
         cache_data_access_time = Sim()->getCfg()->getInt("perf_model/cache/data_access_time");
         cache_tags_access_time = Sim()->getCfg()->getInt("perf_model/cache/tags_access_time");
      }

      virtual ~CachePerfModelBase() { }

      static CachePerfModelBase* createModel(CacheModel_t type);
      static CacheModel_t parseModelType(string model_type);

      virtual UInt32 getLatency(CacheAccess_t access)
      {
         return 0;
      }

};

#endif /* __CACHE_PERF_MODEL_BASE_H__ */
