#ifndef __QUEUE_MODEL_H__
#define __QUEUE_MODEL_H__

#include "fixed_types.h"

class QueueModel
{
public:
   QueueModel();
   ~QueueModel();

   UInt64 max(UInt64 a1, UInt64 a2);
   UInt64 getQueueDelay(UInt64 ref_time, core_id_t requester);
   void updateQueue(UInt64 ref_time, UInt64 processing_time);

private:
   UInt64 m_queue_time;
};

#endif /* __QUEUE_MODEL_H__ */
