#ifndef QUEUE_MODEL_H
#define QUEUE_MODEL_H

class QueueModel
{
public:
   QueueModel()
      : m_queue_time(0),
   {}

   UInt64 getQueueDelay(UInt64 pkt_time)
   {
      return max(m_queue_time - pkt_time, 0);
   }

   void updateQueue(UInt64 pkt_time, UInt64 processing_time)
   {
      m_queue_time = max(m_queue_time, pkt_time) + processing_time;
   }

private:
   UInt64 m_queue_time;
};

#endif /* QUEUE_MODEL_H */
