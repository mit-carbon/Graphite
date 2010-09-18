#ifndef __CACHE_PERF_MODEL_PARALLEL_H__ 
#define __CACHE_PERF_MODEL_PARALLEL_H__

#include "cache_perf_model.h"

class CachePerfModelParallel : public CachePerfModel
{
   private:
      bool m_enabled;

   public:
      CachePerfModelParallel(UInt64 cache_data_access_delay_in_ns,
            UInt64 cache_tags_access_delay_in_ns,
            volatile float core_frequency) : 
         CachePerfModel(cache_data_access_delay_in_ns, cache_tags_access_delay_in_ns, core_frequency),
         m_enabled(false)
      {}
      ~CachePerfModelParallel() {}

      void enable() { m_enabled = true; }
      void disable() { m_enabled = false; }
      bool isEnabled() { return m_enabled; }

      UInt64 getLatency(CacheAccess_t access)
      {
         if (!m_enabled)
            return 0;

         switch(access)
         {
            case ACCESS_CACHE_TAGS:
               return m_cache_tags_access_delay_in_clock_cycles;

            case ACCESS_CACHE_DATA:
            case ACCESS_CACHE_DATA_AND_TAGS:
               return m_cache_data_access_delay_in_clock_cycles;

            default:
               return 0;
         }
      }
 
};

#endif /* __CACHE_PERF_MODEL_PARALLEL_H__ */
