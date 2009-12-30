#include "queue_model_basic.h"
#include "utils.h"
#include "log.h"

QueueModelBasic::QueueModelBasic(bool moving_avg_enabled,
      UInt32 moving_avg_window_size, 
      std::string moving_avg_type_str):
   m_queue_time(0),
   m_moving_average(NULL)
{
   if (moving_avg_enabled)
   {
      MovingAverage<UInt64>::AvgType_t moving_avg_type = MovingAverage<UInt64>::parseAvgType(moving_avg_type_str);
      m_moving_average = MovingAverage<UInt64>::createAvgType(moving_avg_type, moving_avg_window_size);
   }
}

QueueModelBasic::~QueueModelBasic() 
{}

UInt64
QueueModelBasic::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester)
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
   m_queue_time = getMax<UInt64>(m_queue_time, ref_time) + processing_time;

   return queue_delay;
}
