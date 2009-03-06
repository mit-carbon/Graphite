#ifndef QUEUE_MODEL_H
#define QUEUE_MODEL_H

class QueueModel
{
public:
   QueueModel(Core *core)
      : m_core(core)
   {
   }

   UInt64 getQueueDelay()
   {
      UInt64 core_time = Core->getCycleCount();
      return max(m_queue_time - core_time, 0);
   }

   void updateQueue(UInt64 processing_time)
   {
      UInt64 core_time = Core->getCycleCount();
      m_queue_time = max(m_queue_time + processing_time,
                         core_time + processing_time);
   }

private:
   Core *m_core;
   UInt64 m_queue_time;
};

#endif
