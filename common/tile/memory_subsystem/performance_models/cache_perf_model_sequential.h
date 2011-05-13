#ifndef __CACHE_PERF_MODEL_SEQUENTIAL_H__ 
#define __CACHE_PERF_MODEL_SEQUENTIAL_H__

#include "cache_perf_model.h"

class CachePerfModelSequential : public CachePerfModel
{
   private:
      bool m_enabled;

   public:
      CachePerfModelSequential(UInt64 cache_data_access_delay_in_clock_cycles, 
            UInt64 cache_tags_access_delay_in_clock_cycles,
            volatile float core_frequency) : 
         CachePerfModel(cache_data_access_delay_in_clock_cycles, cache_tags_access_delay_in_clock_cycles, core_frequency),
         m_enabled(false)
      {}
      ~CachePerfModelSequential() {}

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
               return m_cache_data_access_delay_in_clock_cycles;

            case ACCESS_CACHE_DATA_AND_TAGS:
               return m_cache_data_access_delay_in_clock_cycles + m_cache_tags_access_delay_in_clock_cycles;

            default:
               return 0;
         }
      }
};

#endif /* __CACHE_PERF_MODEL_SEQUENTIAL_H__ */
