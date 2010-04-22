#ifndef __QUEUE_MODEL_HISTORY_LIST_H__
#define __QUEUE_MODEL_HISTORY_LIST_H__

#include <list>

#include "queue_model.h"
#include "fixed_types.h"

class QueueModelHistoryList : public QueueModel
{
public:
   typedef std::list<std::pair<UInt64,UInt64> > FreeIntervalList;

   QueueModelHistoryList(UInt64 min_processing_time);
   ~QueueModelHistoryList();

   UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester = INVALID_CORE_ID);

   float getQueueUtilization();
   float getFracRequestsUsingAnalyticalModel();

private:
   UInt64 m_min_processing_time;
   UInt32 m_max_free_interval_list_size;

   FreeIntervalList m_free_interval_list;
   
   // Tracks queue utilization
   UInt64 m_utilized_cycles;
   
   // Is analytical model used ?
   bool m_analytical_model_enabled;

   // Performance Counters
   UInt64 m_total_requests;
   UInt64 m_total_requests_using_analytical_model;

   void updateQueueUtilization(UInt64 processing_time);
   UInt64 computeUsingHistoryList(UInt64 pkt_time, UInt64 processing_time);
   void insertInHistoryList(UInt64 pkt_time, UInt64 processing_time);
   UInt64 computeUsingAnalyticalModel(UInt64 pkt_time, UInt64 processing_time);
};

#endif /* __QUEUE_MODEL_HISTORY_LIST_H__ */
