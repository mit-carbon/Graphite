#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "simulator.h"
#include "tile_manager.h"
#include "config.h"
#include "queue_model_history_list.h"
#include "log.h"

QueueModelHistoryList::QueueModelHistoryList(UInt64 min_processing_time)
   : QueueModel(HISTORY_LIST)
   , _min_processing_time(min_processing_time)
{
   // Some Hard-Coded values here
   // Assumptions
   // 1) Simulation Time will not exceed 2^60.
   try
   {
      _max_free_interval_list_size = Sim()->getCfg()->getInt("queue_model/history_list/max_list_size");
      _analytical_model_enabled = Sim()->getCfg()->getBool("queue_model/history_list/analytical_model_enabled");
      _interleaving_enabled = Sim()->getCfg()->getBool("queue_model/history_list/interleaving_enabled");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read parameters from cfg");
   }
   
   _free_interval_list.push_back(std::make_pair<UInt64,UInt64>(0, UINT64_MAX));
   _queue_model_m_g_1 = new QueueModelMG1();

   _total_requests_using_analytical_model = 0;
}

QueueModelHistoryList::~QueueModelHistoryList()
{
   delete _queue_model_m_g_1;
}

UInt64 
QueueModelHistoryList::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester)
{
   LOG_ASSERT_ERROR(_free_interval_list.size() >= 1,
         "Free Interval list size < 1");
 
   UInt64 queue_delay;

   // Check if it is an old packet
   // If yes, use analytical model
   // If not, use the history list based queue model
   std::pair<UInt64,UInt64> oldest_interval = _free_interval_list.front();
   if (_analytical_model_enabled && ((pkt_time + processing_time) < oldest_interval.first))
   {
      // Increment the number of requests that use the analytical model
      _total_requests_using_analytical_model ++;
      queue_delay = _queue_model_m_g_1->computeQueueDelay(pkt_time, processing_time, requester);
   }
   else
   {
      queue_delay = computeUsingHistoryList(pkt_time, processing_time);
   }

   _queue_model_m_g_1->updateQueue(pkt_time, processing_time, queue_delay);
   
   // Update Utilization Counters
   updateQueueUtilizationCounters(pkt_time, processing_time, queue_delay);
   
   return queue_delay;
}

UInt64
QueueModelHistoryList::computeUsingHistoryList(UInt64 pkt_time, UInt64 processing_time)
{
   LOG_ASSERT_ERROR(_free_interval_list.size() <= _max_free_interval_list_size,
         "Free Interval list size(%u) > %u", _free_interval_list.size(), _max_free_interval_list_size);
   UInt64 queue_delay = 0;
 
   FreeIntervalList::iterator curr_it;
   for (curr_it = _free_interval_list.begin(); curr_it != _free_interval_list.end(); curr_it ++)
   {
      std::pair<UInt64,UInt64> interval = (*curr_it);

      if ((pkt_time >= interval.first) && ((pkt_time + processing_time) <= interval.second))
      {
         // No additional queue delay
         // Adjust the data structure accordingly
         curr_it = _free_interval_list.erase(curr_it);
         if ((pkt_time - interval.first) >= _min_processing_time)
         {
            _free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(interval.first, pkt_time));
         }
         if ((interval.second - (pkt_time + processing_time)) >= _min_processing_time)
         {
            _free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(pkt_time + processing_time, interval.second));
         }
         break;
      }
      else if ((pkt_time < interval.first) && ((interval.first + processing_time) <= interval.second))
      {
         // Add additional queue delay
         queue_delay += (interval.first - pkt_time);
         // Adjust the data structure accordingly
         curr_it = _free_interval_list.erase(curr_it);
         if ((interval.second - (interval.first + processing_time)) >= _min_processing_time)
         {
            _free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(interval.first + processing_time, interval.second));
         }
         break;
      }
      else if (_interleaving_enabled)
      {
         if ((pkt_time >= interval.first) && (pkt_time < interval.second))
         {
            curr_it = _free_interval_list.erase(curr_it);
            if ((pkt_time - interval.first) >= _min_processing_time)
            {
               _free_interval_list.insert(curr_it, std::make_pair<UInt64,UInt64>(interval.first, pkt_time));
            }
            curr_it --;
            
            // Adjust times
            pkt_time = interval.second;
            processing_time -= (interval.second - pkt_time);
         }
         else if (pkt_time < interval.first)
         {
            curr_it = _free_interval_list.erase(curr_it);
            curr_it --;
            // Add additional queue delay
            queue_delay += (interval.first - pkt_time);
            
            // Adjust times
            pkt_time = interval.second;
            processing_time -= (interval.second - interval.first);
         }
      }
   }

   if (_free_interval_list.size() > _max_free_interval_list_size)
   {
      _free_interval_list.erase(_free_interval_list.begin());
   }
  
   LOG_PRINT("HistoryList: pkt_time(%llu), processing_time(%llu), queue_delay(%llu)", pkt_time, processing_time, queue_delay);

   return queue_delay;
}
