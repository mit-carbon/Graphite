#include "queue_model.h"

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
QueueModel::getQueueDelay(UInt64 ref_time)
{
   return max(m_queue_time - ref_time, (UInt64) 0);
}

void 
QueueModel::updateQueue(UInt64 ref_time, UInt64 processing_time)
{
   m_queue_time = max(m_queue_time, ref_time) + processing_time;
}
