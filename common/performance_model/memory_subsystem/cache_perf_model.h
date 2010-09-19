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
      UInt64 m_cache_data_access_delay_in_ns;
      UInt64 m_cache_tags_access_delay_in_ns;
      UInt64 m_cache_data_access_delay_in_clock_cycles;
      UInt64 m_cache_tags_access_delay_in_clock_cycles;

   public:
      CachePerfModel(UInt64 cache_data_access_delay_in_ns, UInt64 cache_tags_access_delay_in_ns,
            volatile float core_frequency);
      virtual ~CachePerfModel();

      void updateInternalVariablesOnFrequencyChange(volatile float core_frequency);
      
      static CachePerfModel* create(std::string cache_perf_model_type,
            UInt64 cache_data_access_delay_in_ns,
            UInt64 cache_tags_access_delay_in_ns,
            volatile float core_frequency);
      static PerfModel_t parseModelType(std::string model_type);

      virtual void enable() = 0;
      virtual void disable() = 0;
      virtual bool isEnabled() = 0;

      virtual UInt64 getLatency(CacheAccess_t access) = 0;
};

#endif /* __CACHE_PERF_MODEL_H__ */
