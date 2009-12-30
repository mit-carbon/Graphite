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

private:
   UInt64 m_min_processing_time;
   UInt32 m_max_free_interval_list_size;

   FreeIntervalList m_free_interval_list;
};

#endif /* __QUEUE_MODEL_HISTORY_LIST_H__ */
