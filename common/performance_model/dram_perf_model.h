#ifndef __DRAM_PERF_MODEL_H__
#define __DRAM_PERF_MODEL_H__

#include "queue_model.h"
#include "core.h"
#include "fixed_types.h"

// Note: Each Dram Controller owns a single DramModel object
// Hence, m_dram_bandwidth is the bandwidth for a single DRAM controller
// Total Bandwidth = m_dram_bandwidth * Number of DRAM controllers
// Number of DRAM controllers presently = Number of Cores
// m_dram_bandwidth is expressed in GB/s
// Assuming the frequency of a core is 1GHz, 
// m_dram_bandwidth is also expressed in 'Bytes per clock cycle'
// This DRAM model is not entirely correct.
// It sort of increases the queueing delay to a huge value if
// the arrival times of adjacent packets are spread over a large
// simulated time period
class DramPerfModel
{
   private:
      QueueModel* m_queue_model;
      Core* m_core;
      UInt32 m_dram_access_cost;
      UInt32 m_dram_bandwidth;
      bool m_enabled;

   public:
      DramPerfModel(Core* core);
      ~DramPerfModel();

      Core* getCore() { return m_core; }
      UInt64 getAccessLatency(UInt64 pkt_time, UInt64 pkt_size);
      void enable();
      void disable();
};

#endif /* __DRAM_PERF_MODEL_H__ */
