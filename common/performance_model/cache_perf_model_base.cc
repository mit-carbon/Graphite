#include <string>

#include "cache_perf_model_base.h"
#include "cache_perf_model_parallel.h"
#include "cache_perf_model_sequential.h"
#include "log.h"

using namespace std;

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

CachePerfModelBase::CacheModel_t 
CachePerfModelBase::parseModelType(string model_type)
{
   if (model_type == "parallel")
   {
      return CACHE_MODEL_PARALLEL;
   }

   else if (model_type == "sequential")
   {
      return CACHE_MODEL_SEQUENTIAL;
   }

   else 
   {
      LOG_PRINT_ERROR("Unsupported CacheModel type: %s", model_type.c_str());
      return NUM_CACHE_MODELS;
   }
}

