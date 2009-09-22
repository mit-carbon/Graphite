#include <iostream>
using namespace std;

#include "dram_perf_model.h"

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
DramPerfModel::DramPerfModel(float dram_access_cost, 
      float dram_bandwidth,
      float core_frequency, 
      bool queue_model_enabled, 
      UInt32 moving_avg_window_size, 
      std::string moving_avg_type_str):
   m_queue_model_enabled(queue_model_enabled),
   m_enabled(true),
   m_num_accesses(0),
   m_total_access_latency(0.0),
   m_total_queueing_delay(0.0)
{
   // Convert everything to be in 'clock cycles' instead of 'seconds'
   m_dram_access_cost = (UInt32) (dram_access_cost * core_frequency);
   m_dram_bandwidth = dram_bandwidth / core_frequency;

   MovingAverage<UInt64>::AvgType_t moving_avg_type = MovingAverage<UInt64>::parseAvgType(moving_avg_type_str);
   m_moving_average = MovingAverage<UInt64>::createAvgType(moving_avg_type, moving_avg_window_size);
   m_queue_model = new QueueModel();
}

DramPerfModel::~DramPerfModel()
{
   delete m_queue_model;
}

UInt64 
DramPerfModel::getAccessLatency(UInt64 pkt_time, UInt64 pkt_size)
{
   // pkt_size is in 'Bytes'
   // m_dram_bandwidth is in 'Bytes per clock cycle'
   if (!m_enabled)
      return 0;

   UInt64 processing_time = (UInt64) ((float) pkt_size/m_dram_bandwidth) + 1;

   // Compute the moving average here
   UInt64 pkt_time_av = m_moving_average->compute(pkt_time);

   UInt64 queue_delay;
   
   if (m_queue_model_enabled)
   {
      queue_delay = m_queue_model->getQueueDelay(pkt_time_av);
      m_queue_model->updateQueue(pkt_time_av, processing_time);
   }
   else
   {
      queue_delay = 0;
   }

   UInt64 access_latency = queue_delay + processing_time + m_dram_access_cost;

   // Update Memory Counters
   m_num_accesses ++;
   m_total_access_latency += (double) access_latency;
   m_total_queueing_delay += (double) queue_delay;

   LOG_PRINT("getAccessLatency(): Queue Delay: %llu", queue_delay);
   
   return access_latency;
}

void
DramPerfModel::outputSummary(ostream& out)
{
   out << "Dram Perf Model summary: " << endl;
   out << "    num dram accesses: " << m_num_accesses << endl;
   out << "    average dram access latency: " << 
      (float) (m_total_access_latency / m_num_accesses) << endl;
   out << "    average dram queueing delay: " << 
      (float) (m_total_queueing_delay / m_num_accesses) << endl;
}
