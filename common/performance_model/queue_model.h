#ifndef __QUEUE_MODEL_H__
#define __QUEUE_MODEL_H__

#include "fixed_types.h"

class QueueModel
{
public:
   QueueModel()
      : m_queue_time(0)
   {}
   ~QueueModel() {}

   UInt64 max(UInt64 a1, UInt64 a2)
   {
      return (a1 > a2) ? a1 : a2;
   }

   UInt64 getQueueDelay(UInt64 pkt_time)
   {
      // FIXME: Figure out a way to get this core_time
      return max(m_queue_time - pkt_time, (UInt64) 0);
   }

   void updateQueue(UInt64 pkt_time, UInt64 processing_time)
   {
      m_queue_time = max(m_queue_time, pkt_time) + processing_time;
   }

   void resetModel()
   {
      m_queue_time = 0;
   }

private:
   UInt64 m_queue_time;
};

#endif /* __QUEUE_MODEL_H__ */
