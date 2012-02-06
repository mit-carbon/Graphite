#include <string>
#include <cmath>

#include "cache_perf_model.h"
#include "cache_perf_model_parallel.h"
#include "cache_perf_model_sequential.h"
#include "log.h"

CachePerfModel::CachePerfModel(UInt64 data_array_access_time, UInt64 tag_array_access_time, float core_frequency)
   : _data_array_access_time(data_array_access_time)
   , _tag_array_access_time(tag_array_access_time)
{}

CachePerfModel::~CachePerfModel()
{}

CachePerfModel* 
CachePerfModel::create(std::string cache_perf_model_type, UInt64 data_array_access_time,
                       UInt64 tag_array_access_time, float core_frequency)
{
   PerfModel_t perf_model = parseModelType(cache_perf_model_type);

   switch(perf_model)
   {
      case(CACHE_PERF_MODEL_PARALLEL):
         return new CachePerfModelParallel(data_array_access_time, tag_array_access_time, core_frequency);
            
      case(CACHE_PERF_MODEL_SEQUENTIAL):
         return new CachePerfModelSequential(data_array_access_time, tag_array_access_time, core_frequency);
            
      default:
         LOG_ASSERT_ERROR(false, "Unsupported CachePerfModel type: %s", cache_perf_model_type.c_str());
         return NULL;
   }
}

CachePerfModel::PerfModel_t 
CachePerfModel::parseModelType(std::string model_type)
{
   if (model_type == "parallel")
      return CACHE_PERF_MODEL_PARALLEL;
   else if (model_type == "sequential")
      return CACHE_PERF_MODEL_SEQUENTIAL;
   else 
      LOG_PRINT_ERROR("Unsupported CacheModel type: %s", model_type.c_str());
   return NUM_CACHE_PERF_MODELS;
}
