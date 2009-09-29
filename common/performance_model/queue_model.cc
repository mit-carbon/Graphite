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
QueueModel::getQueueDelay(UInt64 ref_time, core_id_t requester)
{
   UInt64 queue_delay = (m_queue_time > ref_time) ? (m_queue_time - ref_time) : 0;
   LOG_PRINT("Queue Time = %llu, Ref Time = %llu, Queue Delay = %llu, Requester(%i)", 
         m_queue_time, ref_time, queue_delay, requester);
   return queue_delay;
}

void 
QueueModel::updateQueue(UInt64 ref_time, UInt64 processing_time)
{
   m_queue_time = max(m_queue_time, ref_time) + processing_time;
}
