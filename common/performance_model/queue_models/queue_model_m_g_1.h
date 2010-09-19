#pragma once

#include "fixed_types.h"

class QueueModelMG1
{
public:
   QueueModelMG1();
   ~QueueModelMG1();

   UInt64 computeQueueDelay(UInt64 pkt_time, UInt64 service_time, core_id_t requester = INVALID_CORE_ID);
   void updateQueue(UInt64 pkt_time, UInt64 service_time, UInt64 waiting_time_queue);

private:
   // Service Time distribution Parameters
   volatile double _sigma_service_time_square;
   volatile double _sigma_service_time;
   UInt64 _num_arrivals;
   UInt64 _newest_arrival_time;

   volatile double square(volatile double num) { return num*num; }
};
