#include "cache_perf_model_base.h"
#include "cache_perf_model_parallel.h"
#include "cache_perf_model_sequential.h"
#include "log.h"

CachePerfModelBase* CachePerfModelBase::createModel(CacheModel_t type)
{
   switch(type)
   {
      case(CACHE_MODEL_PARALLEL):
         return new CachePerfModelParallel();
            
      case(CACHE_MODEL_SEQUENTIAL):
         return new CachePerfModelSequential();
            
      default:
         LOG_ASSERT_ERROR(false, "Unsupported CacheModel type: %u", type);
         return NULL;
   }
}
