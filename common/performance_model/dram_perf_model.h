#ifndef __DRAM_PERF_MODEL_H__
#define __DRAM_PERF_MODEL_H__

#include "queue_model.h"
#include "utils.h"

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
      UInt32 m_dram_access_cost;
      UInt32 m_dram_bandwidth;

   public:
      DramPerfModel()
      {
         try
         {
            m_dram_access_cost = Sim()->getCfg()->getInt("perf_model/dram/access_cost");
            m_dram_bandwidth = Sim()->getCfg()->getInt("perf_model/dram/bandwidth_per_controller");
         }
         catch(...)
         {
            LOG_PRINT_ERROR("Dram parameter obtained a bad value from config.");
         }

         m_queue_model = new QueueModel();
      }

      ~DramPerfModel()
      {
         delete m_queue_model;
      }

      UInt64 getAccessLatency(UInt64 pkt_time, UInt64 pkt_size)
      {
         // pkt_size is in 'Bytes'
         // m_dram_bandwidth is in 'Bytes per clock cycle'
         UInt64 processing_time = pkt_size/m_dram_bandwidth + 1;

         UInt64 queue_delay = (UInt64) 0;
         // UInt64 queue_delay = m_queue_model->getQueueDelay(pkt_time);
         // m_queue_model->updateQueue(pkt_time, processing_time);

         return (queue_delay + processing_time + m_dram_access_cost);
      }

      void resetModel()
      {
         m_queue_model->resetModel();
      }
};

#endif /* __DRAM_PERF_MODEL_H__ */
