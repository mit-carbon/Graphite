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
   : m_enabled(true)
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

   UInt64 processing_time = pkt_size/m_dram_bandwidth + 1;

   UInt64 core_time = getCore()->getPerformanceModel()->getCycleCount();
   UInt64 queue_delay = m_queue_model->getQueueDelay(core_time);
   m_queue_model->updateQueue(core_time, processing_time);

   return (queue_delay + processing_time + m_dram_access_cost);
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
