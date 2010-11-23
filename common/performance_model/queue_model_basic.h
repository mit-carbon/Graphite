#ifndef __QUEUE_MODEL_BASIC_H__
#define __QUEUE_MODEL_BASIC_H__

#include "queue_model.h"
#include "moving_average.h"
#include "fixed_types.h"

class QueueModelBasic : public QueueModel
{
public:
   QueueModelBasic(bool moving_avg_enabled, UInt32 moving_avg_window_size, std::string moving_avg_type_str);
   ~QueueModelBasic();

   UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester = INVALID_TILE_ID);

private:
   UInt64 m_queue_time;
   MovingAverage<UInt64>* m_moving_average;
};

#endif /* __QUEUE_MODEL_BASIC_H__ */
