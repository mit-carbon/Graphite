#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "queue_model_history_list.h"
#include "fxsupport.h"
#include "log.h"

QueueModelHistoryList::QueueModelHistoryList(UInt64 min_processing_time):
   m_min_processing_time(min_processing_time),
   m_utilized_cycles(0),
   m_total_requests(0),
   m_total_requests_using_analytical_model(0)
{
   // Some Hard-Coded values here
   // Assumptions
   // 1) Simulation Time will not exceed 2^60.
   try
   {
      m_analytical_model_enabled = Sim()->getCfg()->getBool("queue_model/history_list/analytical_model_enabled");
      m_max_free_interval_list_size = Sim()->getCfg()->getInt("queue_model/history_list/max_list_size");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read parameters from cfg");
   }
   UInt64 max_simulation_time = ((UInt64) 1) << 60;
   m_free_interval_list.push_back(std::make_pair<UInt64,UInt64>((UInt64) 0, max_simulation_time));
}

QueueModelHistoryList::~QueueModelHistoryList()
{}

UInt64 
QueueModelHistoryList::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester)
{
   LOG_ASSERT_ERROR(m_free_interval_list.size() >= 1,
         "Free Interval list size < 1");
 
   UInt64 queue_delay;

   // Check if it is an old packet
   // If yes, use analytical model
   // If not, use the history list based queue model
   std::pair<UInt64,UInt64> oldest_interval = m_free_interval_list.front();
   if (m_analytical_model_enabled && ((pkt_time + processing_time) <= oldest_interval.first))
   {
      // Increment the number of requests that use the analytical model
      m_total_requests_using_analytical_model ++;
      queue_delay = computeUsingAnalyticalModel(pkt_time, processing_time);
   }
   else
   {
      queue_delay = computeUsingHistoryList(pkt_time, processing_time);
   }

   updateQueueUtilization(processing_time);

   // Increment total queue requests
   m_total_requests ++;
   
   return queue_delay;
}

float
QueueModelHistoryList::getQueueUtilization()
{
   std::pair<UInt64,UInt64> newest_interval = m_free_interval_list.back();
   UInt64 total_cycles = newest_interval.first;

   if (total_cycles == 0)
   {
      LOG_ASSERT_ERROR(m_utilized_cycles == 0, "m_utilized_cycles(%llu), m_total_cycles(%llu)",
            m_utilized_cycles, total_cycles);
      return 0;
   }
   else
   {
      return ((float) m_utilized_cycles / total_cycles);
   } 
}
   
float
QueueModelHistoryList::getFracRequestsUsingAnalyticalModel() 
{
  if (m_total_requests == 0)
     return 0;
  else
     return ((float) m_total_requests_using_analytical_model / m_total_requests); 
}

void
QueueModelHistoryList::updateQueueUtilization(UInt64 processing_time)
{
   // Update queue utilization parameter
   m_utilized_cycles += processing_time;
}

UInt64
QueueModelHistoryList::computeUsingAnalyticalModel(UInt64 pkt_time, UInt64 processing_time)
{
   // processing_time = number of packet flits
   volatile float rho = getQueueUtilization();
   
   UInt64 queue_delay = (UInt64) (((rho * processing_time) / (2 * (1 - rho))) + 1);

   LOG_ASSERT_ERROR(queue_delay < 10000000, "queue_delay(%llu), pkt_time(%llu), processing_time(%llu), rho(%f)",
         queue_delay, pkt_time, processing_time, rho);
  
   // This can be done in a more efficient way. Doing it in the most stupid way now
   insertInHistoryList(pkt_time, processing_time);

   LOG_PRINT("AnalyticalModel: pkt_time(%llu), processing_time(%llu), queue_delay(%llu)", pkt_time, processing_time, queue_delay);

   return queue_delay;
}

void
QueueModelHistoryList::insertInHistoryList(UInt64 pkt_time, UInt64 processing_time)
{
   __attribute((unused)) UInt64 queue_delay = computeUsingHistoryList(pkt_time, processing_time);
}

UInt64
QueueModelHistoryList::computeUsingHistoryList(UInt64 pkt_time, UInt64 processing_time)
{
   LOG_ASSERT_ERROR(m_free_interval_list.size() <= m_max_free_interval_list_size,
         "Free Interval list size(%u) > %u", m_free_interval_list.size(), m_max_free_interval_list_size);
   UInt64 queue_delay = UINT64_MAX;
  
   FreeIntervalList::iterator curr_it;
   for (curr_it = m_free_interval_list.begin(); curr_it != m_free_interval_list.end(); curr_it ++)
   {
      std::pair<UInt64,UInt64> interval = (*curr_it);

      if ((pkt_time >= interval.first) && ((pkt_time + processing_time) <= interval.second))
      {
         queue_delay = 0;
         // Adjust the data structure accordingly
         curr_it = m_free_interval_list.erase(curr_it);
         if ((pkt_time - interval.first) >= m_min_processing_time)
         {
            m_free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(interval.first, pkt_time));
         }
         if ((interval.second - (pkt_time + processing_time)) >= m_min_processing_time)
         {
            m_free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(pkt_time + processing_time, interval.second));
         }
         break;
      }
      else if ((pkt_time < interval.first) && ((interval.first + processing_time) <= interval.second))
      {
         queue_delay = interval.first - pkt_time;
         // Adjust the data structure accordingly
         curr_it = m_free_interval_list.erase(curr_it);
         if ((interval.second - (interval.first + processing_time)) >= m_min_processing_time)
         {
            m_free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(interval.first + processing_time, interval.second));
         }
         break;
      }
   }

   LOG_ASSERT_ERROR(queue_delay != UINT64_MAX, "queue delay(%llu), free interval not found", queue_delay);

   if (m_free_interval_list.size() > m_max_free_interval_list_size)
   {
      m_free_interval_list.erase(m_free_interval_list.begin());
   }
  
   LOG_PRINT("HistoryList: pkt_time(%llu), processing_time(%llu), queue_delay(%llu)", pkt_time, processing_time, queue_delay);

   return queue_delay;
}
