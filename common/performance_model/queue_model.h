#ifndef __QUEUE_MODEL_H__
#define __QUEUE_MODEL_H__

#include <iostream>

#include "fixed_types.h"

class QueueModel
{
public:
   QueueModel() {}
   virtual ~QueueModel() {}

   virtual UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, core_id_t requester = INVALID_CORE_ID) = 0;

   static QueueModel* create(std::string model_type, UInt64 min_processing_time);
};

#endif /* __QUEUE_MODEL_H__ */
