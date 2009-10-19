#ifndef __QUEUE_MODEL_H__
#define __QUEUE_MODEL_H__

#include "moving_average.h"
#include "fixed_types.h"

class QueueModel
{
public:
   QueueModel(bool moving_avg_enabled, UInt32 moving_avg_window_size, std::string moving_avg_type_str);
   ~QueueModel();

   UInt64 computeQueueDelay(UInt64 ref_time, UInt64 processing_time, core_id_t requester = INVALID_CORE_ID);

private:
   UInt64 m_queue_time;
   MovingAverage<UInt64>* m_moving_average;
};

#endif /* __QUEUE_MODEL_H__ */
