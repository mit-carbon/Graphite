#pragma once

#include "queue_model.h"
#include "moving_average.h"
#include "fixed_types.h"

class QueueModelBasic : public QueueModel
{
public:
   QueueModelBasic();
   ~QueueModelBasic();

   UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester = INVALID_TILE_ID);

private:
   UInt64 _queue_time;
   MovingAverage<UInt64>* _moving_average;
};
