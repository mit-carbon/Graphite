#include <string>
#include <cmath>

#include "cache_perf_model.h"
#include "cache_perf_model_parallel.h"
#include "cache_perf_model_sequential.h"
#include "log.h"
#include "dvfs_manager.h"

CachePerfModel::CachePerfModel(UInt64 data_access_cycles, UInt64 tags_access_cycles, float frequency)
   : _data_access_cycles(data_access_cycles)
   , _tags_access_cycles(tags_access_cycles)
{ 
   _data_access_latency = Time(Latency(data_access_cycles, frequency));
   _tags_access_latency = Time(Latency(tags_access_cycles, frequency));
   _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), frequency));
}


CachePerfModel::~CachePerfModel()
{}

CachePerfModel* 
CachePerfModel::create(string perf_model_type, UInt64 data_access_cycles, UInt64 tags_access_cycles, float frequency)
{
   ModelType model_type = parseModelType(perf_model_type);

   switch (model_type)
   {
      case PARALLEL:
         return new CachePerfModelParallel(data_access_cycles, tags_access_cycles, frequency);
            
      case SEQUENTIAL:
         return new CachePerfModelSequential(data_access_cycles, tags_access_cycles, frequency);
            
      default:
         LOG_PRINT_ERROR("Unsupported Cache Performance Model Type: %s", perf_model_type.c_str());
         return NULL;
   }
}

void
CachePerfModel::setDVFS(double frequency)
{
   _data_access_latency = Time(Latency(_data_access_cycles, frequency));
   _tags_access_latency = Time(Latency(_tags_access_cycles, frequency));
   _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), frequency));
}

CachePerfModel::ModelType 
CachePerfModel::parseModelType(string model_type)
{
   if (model_type == "parallel")
      return PARALLEL;
   else if (model_type == "sequential")
      return SEQUENTIAL;
   else 
      LOG_PRINT_ERROR("Unsupported Cache Model Type: %s", model_type.c_str());
   return NUM_MODEL_TYPES;
}
