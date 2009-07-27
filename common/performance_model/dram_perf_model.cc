#include "dram_perf_model.h"
#include "simulator.h"
#include "log.h"

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
DramPerfModel::DramPerfModel(Core* core) 
   : m_enabled(true),
   m_num_accesses(0),
   m_total_access_latency(0.0),
   m_total_queueing_delay(0.0)
{
   UInt32 moving_avg_window_size;
   MovingAverage<UInt64>::AvgType_t moving_avg_type;

   UInt32 total_cores = Sim()->getConfig()->getTotalCores();
   try
   {
      m_queueing_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queueing_model_enabled");
      m_dram_access_cost = Sim()->getCfg()->getInt("perf_model/dram/access_cost");
      m_dram_bandwidth =  Sim()->getCfg()->getFloat("perf_model/dram/offchip_bandwidth") / total_cores;
      moving_avg_window_size = Sim()->getCfg()->getInt("perf_model/dram/moving_avg_window_size");
      moving_avg_type = MovingAverage<UInt64>::parseAvgType(Sim()->getCfg()->getString("perf_model/dram/moving_avg_type"));
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Dram parameter obtained a bad value from config.");
      return;
   }

   m_moving_average = MovingAverage<UInt64>::createAvgType(moving_avg_type, moving_avg_window_size);
   m_queue_model = new QueueModel();
   m_core = core;
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
   
   if (m_queueing_model_enabled)
   {
      queue_delay = m_queue_model->getQueueDelay(pkt_time_av);
      m_queue_model->updateQueue(pkt_time_av, processing_time);
   }
   else
   {
      queue_delay = 0;
   }

   UInt64 access_latency = queue_delay + processing_time + m_dram_access_cost;

   // Update Dram Counters
   m_num_accesses ++;
   m_total_access_latency += (double) access_latency;
   m_total_queueing_delay += (double) queue_delay;
   
   __attribute(__unused__) UInt64 core_time = getCore()->getPerformanceModel()->getCycleCount();

   LOG_PRINT("Pkt Size = %llu, Dram Bandwidth = %f, Processing Time = %i", 
         pkt_size, m_dram_bandwidth, processing_time);
   LOG_PRINT("Av Pkt Time = %llu, Core Time = %llu", pkt_time_av, core_time);
   LOG_PRINT("Dram Queue Delay = %llu", queue_delay);
   return access_latency;
}

void 
DramPerfModel::enable()
{
   m_enabled = true;
}

void 
DramPerfModel::disable()
{
   m_enabled = false;
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
