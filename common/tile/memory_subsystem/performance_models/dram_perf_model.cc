#include <iostream>
using namespace std;

#include "simulator.h"
#include "config.h"
#include "dram_perf_model.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"
#include "constants.h"

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
DramPerfModel::initializePerformanceCounters()
{
   m_num_accesses = 0;
   m_total_access_latency = 0;
   m_total_queueing_delay = 0;
}

Latency 
DramPerfModel::getAccessLatency(Time pkt_time, UInt64 pkt_size)
{

   // In the following we assume a 1GHz frequency, so that
   // 1 cycle = 1 nanosecond.
   
   // convert to nanoseconds
   UInt64 pkt_time_ns = (UInt64) ceil(pkt_time.getTime()/1000.0);

   // pkt_size is in 'Bytes'
   // m_dram_bandwidth is in 'Bytes per clock cycle'
   if (!m_enabled) 
   {
      LOG_PRINT("Not enabled. Return 0");
      return Latency(0,DRAM_FREQUENCY);
   }

   UInt64 processing_time = (UInt64) ((float) pkt_size/m_dram_bandwidth) + 1;
   LOG_PRINT("Processing Time(%llu)", processing_time);

   // Compute Queue Delay
   UInt64 queue_delay;
   if (m_queue_model)
   {
      queue_delay = m_queue_model->computeQueueDelay(pkt_time_ns, processing_time);
   }
   else
   {
      queue_delay = 0;
   }
   LOG_PRINT("Queue Delay(%llu)", queue_delay);
   UInt64 access_latency = queue_delay + processing_time + m_dram_access_cost;
   LOG_PRINT("Access Latency(%llu)", access_latency);


   // Update Memory Counters
   m_num_accesses ++;
   m_total_access_latency += (double) access_latency;
   m_total_queueing_delay += (double) queue_delay;

   return Latency(access_latency,DRAM_FREQUENCY);
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
   out << "Dram Performance Model Summary: " << endl;
   out << "    Total Dram Accesses: " << m_num_accesses << endl;
   out << "    Average Dram Access Latency (in nanoseconds): " << 
      (float) (m_total_access_latency / m_num_accesses) << endl;
   out << "    Average Dram Contention Delay (in nanoseconds): " << 
      (float) (m_total_queueing_delay / m_num_accesses) << endl;


   std::string queue_model_type = Sim()->getCfg()->getString("dram/queue_model/type");
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
   out << "Dram Performance Model Summary: " << endl;
   out << "    Total Dram Accesses: " << endl;
   out << "    Average Dram Access Latency (in nanoseconds): " << endl;
   out << "    Average Dram Contention Delay (in nanoseconds): " << endl;
   
   bool queue_model_enabled = Sim()->getCfg()->getBool("dram/queue_model/enabled");
   std::string queue_model_type = Sim()->getCfg()->getString("dram/queue_model/type");
   if (queue_model_enabled && ((queue_model_type == "history_list") || (queue_model_type == "history_tree")))
   {
      out << "    Queue Model:" << endl;
      out << "      Queue Utilization(\%): " << endl;
      out << "      Analytical Model Used(\%): " << endl;
   }
}
