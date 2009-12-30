#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "queue_model_history_list.h"
#include "log.h"

QueueModelHistoryList::QueueModelHistoryList(UInt64 min_processing_time):
   m_min_processing_time(min_processing_time)
{
   // Some Hard-Coded values here
   // Assumptions
   // 1) Simulation Time will not exceed 2^60.
   // 2) We limit the free interval list size to 10,000
   m_max_free_interval_list_size = 10000;
   UInt64 max_simulation_time = ((UInt64) 1) << 60;
   m_free_interval_list.push_back(std::make_pair<UInt64,UInt64>((UInt64) 0, max_simulation_time));
}

QueueModelHistoryList::~QueueModelHistoryList()
{}

UInt64 
QueueModelHistoryList::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester)
{
   LOG_ASSERT_ERROR(m_free_interval_list.size() <= m_max_free_interval_list_size,
         "Free Interval list size > %u", m_max_free_interval_list_size);
   LOG_ASSERT_ERROR(m_free_interval_list.size() >= 1,
         "Free Interval list size < 1");
  
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
   
   return queue_delay;
}
