#ifndef __QUEUE_MODEL_HISTORY_LIST_H__
#define __QUEUE_MODEL_HISTORY_LIST_H__

#include <list>

#include "queue_model.h"
#include "queue_model_m_g_1.h"
#include "fixed_types.h"

class QueueModelHistoryList : public QueueModel
{
public:
   QueueModelHistoryList(UInt64 min_processing_time);
   ~QueueModelHistoryList();

   UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester = INVALID_TILE_ID);
   UInt64 getTotalRequestsUsingAnalyticalModel() { return _total_requests_using_analytical_model; }

private:
   typedef std::list<std::pair<UInt64,UInt64> > FreeIntervalList;

   QueueModelMG1* _queue_model_m_g_1;
   FreeIntervalList _free_interval_list;
   
   // Is analytical model used ?
   bool _analytical_model_enabled;
   
   UInt64 _min_processing_time;
   UInt32 _max_free_interval_list_size;
   bool _interleaving_enabled;

   // Tracks queue utilization
   UInt64 _utilized_cycles;
   // Performance Counters
   UInt64 _total_requests;
   UInt64 _total_requests_using_analytical_model;

   UInt64 computeUsingHistoryList(UInt64 pkt_time, UInt64 processing_time);
   void insertInHistoryList(UInt64 pkt_time, UInt64 processing_time);
};

#endif /* __QUEUE_MODEL_HISTORY_LIST_H__ */
