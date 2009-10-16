#include "queue_model.h"
#include "log.h"

QueueModel::QueueModel()
   : m_queue_time(0)
{}

QueueModel::~QueueModel() {}

UInt64 
QueueModel::max(UInt64 a1, UInt64 a2)
{
   return (a1 > a2) ? a1 : a2;
}

UInt64 
QueueModel::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester)
{
   // Compute the moving average here
   UInt64 ref_time;
   if (m_moving_average)
   {
      ref_time = m_moving_average->compute(pkt_time);
   }
   else
   {
      ref_time = pkt_time;
   }

   UInt64 queue_delay = (m_queue_time > ref_time) ? (m_queue_time - ref_time) : 0;
   if (queue_delay > 10000)
   {
      LOG_PRINT("Queue Time(%llu), Ref Time(%llu), Queue Delay(%llu), Requester(%i)", 
         m_queue_time, ref_time, queue_delay, requester);
   }
   else if ((queue_delay == 0) && ((ref_time - m_queue_time) > 10000))
   {
      LOG_PRINT("Queue Time(%llu), Ref Time(%llu), Difference(%llu), Requester(%i)",
            m_queue_time, ref_time, ref_time - m_queue_time, requester);
   }
   
   // Update the Queue Time
   m_queue_time = max(m_queue_time, ref_time) + processing_time;

   return queue_delay;
}
