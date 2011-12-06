#include <iostream>
using namespace std;

#include "simulator.h"
#include "config.h"
#include "dram_perf_model.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"

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
      bool queue_model_enabled,
      std::string queue_model_type, 
      UInt32 cache_block_size):
   m_dram_access_cost(UInt64(dram_access_cost)),
   m_dram_bandwidth(dram_bandwidth),
   m_cache_block_size(cache_block_size),
   m_queue_model_type(queue_model_type),
   m_queue_model_enabled(queue_model_enabled),
   m_enabled(false)
{
   initializePerformanceCounters();
   createQueueModels();
}

DramPerfModel::~DramPerfModel()
{
   destroyQueueModels();
}

void
DramPerfModel::createQueueModels()
{
   if (m_queue_model_enabled)
   {
      UInt64 min_processing_time = (UInt64) ((float) m_cache_block_size / m_dram_bandwidth) + 1;
      m_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   else
   {
      m_queue_model = NULL;
   }
}

void
DramPerfModel::destroyQueueModels()
{
   if (m_queue_model_enabled)
   {
      delete m_queue_model;
   }
}

void
DramPerfModel::resetQueueModels()
{
   destroyQueueModels();
   createQueueModels();
}

void
DramPerfModel::initializePerformanceCounters()
{
   m_num_accesses = 0;
   m_total_access_latency = 0;
   m_total_queueing_delay = 0;
}

UInt64 
DramPerfModel::getAccessLatency(UInt64 pkt_time, UInt64 pkt_size, tile_id_t requester)
{
   // pkt_size is in 'Bytes'
   // m_dram_bandwidth is in 'Bytes per clock cycle'
   if ((!m_enabled) || 
         (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()))
   {
      return 0;
   }

   UInt64 processing_time = (UInt64) ((float) pkt_size/m_dram_bandwidth) + 1;

   // Compute Queue Delay
   UInt64 queue_delay;
   if (m_queue_model)
   {
      queue_delay = m_queue_model->computeQueueDelay(pkt_time, processing_time, requester);
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
DramPerfModel::reset()
{
   initializePerformanceCounters();
   resetQueueModels();
}

void
DramPerfModel::outputSummary(ostream& out)
{
   out << "Dram Perf Model summary: " << endl;
   out << "    Total Dram Accesses: " << m_num_accesses << endl;
   out << "    Average Dram Access Latency: " << 
      (float) (m_total_access_latency / m_num_accesses) << endl;
   out << "    Average Dram Contention Delay: " << 
      (float) (m_total_queueing_delay / m_num_accesses) << endl;
   
   std::string queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");
   if (m_queue_model && ((queue_model_type == "history_list") || (queue_model_type == "history_tree")))
   {
      out << "    Queue Model:" << endl;
       
      if (queue_model_type == "history_list")
      {  
         float queue_utilization = ((QueueModelHistoryList*) m_queue_model)->getQueueUtilization();
         float frac_requests_using_analytical_model = \
            ((float) ((QueueModelHistoryList*) m_queue_model)->getTotalRequestsUsingAnalyticalModel()) / \
            ((QueueModelHistoryList*) m_queue_model)->getTotalRequests();
         out << "      Queue Utilization(\%): " << queue_utilization * 100 << endl;
         out << "      Analytical Model Used(\%): " << frac_requests_using_analytical_model * 100 << endl;
      }
      else // (queue_model_type == "history_tree")
      {
         float queue_utilization = ((QueueModelHistoryTree*) m_queue_model)->getQueueUtilization();
         float frac_requests_using_analytical_model = \
            ((float) ((QueueModelHistoryTree*) m_queue_model)->getTotalRequestsUsingAnalyticalModel()) / \
            ((QueueModelHistoryTree*) m_queue_model)->getTotalRequests();
         out << "      Queue Utilization(\%): " << queue_utilization * 100 << endl;
         out << "      Analytical Model Used(\%): " << frac_requests_using_analytical_model * 100 << endl;
      }
   }
}

void
DramPerfModel::dummyOutputSummary(ostream& out)
{
   out << "Dram Performance Model summary: " << endl;
   out << "    Total Dram Accesses: " << endl;
   out << "    Average Dram Access Latency: " << endl;
   out << "    Average Dram Contention Delay: " << endl;
   
   bool queue_model_enabled = Sim()->getCfg()->getBool("perf_model/dram/queue_model/enabled");
   std::string queue_model_type = Sim()->getCfg()->getString("perf_model/dram/queue_model/type");
   if (queue_model_enabled && ((queue_model_type == "history_list") || (queue_model_type == "history_tree")))
   {
      out << "    Queue Model:" << endl;
      out << "      Queue Utilization(\%): " << endl;
      out << "      Analytical Model Used(\%): " << endl;
   }
}
