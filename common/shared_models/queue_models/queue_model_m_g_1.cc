#include <cmath>

#include "queue_model_m_g_1.h"
#include "utils.h"
#include "log.h"

QueueModelMG1::QueueModelMG1():
   _sigma_service_time_square(0.0),
   _sigma_service_time(0.0),
   _num_arrivals(0),
   _newest_arrival_time(0)
{}

QueueModelMG1::~QueueModelMG1()
{}
   
UInt64
QueueModelMG1::computeQueueDelay(UInt64 pkt_time, UInt64 service_time, tile_id_t requester)
{
   LOG_ASSERT_ERROR(service_time > 0, "service_time(%llu)", service_time);

   UInt64 waiting_time_queue;

   // processing_time = number of packet flits
   if (_num_arrivals != 0)
   {
      double variance_service_time = ( (_sigma_service_time_square / _num_arrivals) - \
                                              square(_sigma_service_time / _num_arrivals) );
      double service_rate = 1.0 / (_sigma_service_time / _num_arrivals);
      double arrival_rate = ((double) _num_arrivals) / _newest_arrival_time;

      LOG_PRINT("variance_serve_time(%g), service_rate(%g), arrival_rate(%g)\n", variance_service_time, service_rate, arrival_rate);
      if (arrival_rate >= service_rate)
         arrival_rate = 0.999 * service_rate;
      
      waiting_time_queue = (UInt64) ceil(0.5 * service_rate * arrival_rate * ( (1 / square(service_rate)) + variance_service_time) / (service_rate - arrival_rate));
   }
   else
   {
      waiting_time_queue = 0;
   }

   LOG_PRINT("MG1Model: pkt_time(%llu), service_time(%llu), waiting_time_queue(%llu)", pkt_time, service_time, waiting_time_queue);

   return waiting_time_queue;
}

void
QueueModelMG1::updateQueue(UInt64 pkt_time, UInt64 service_time, UInt64 waiting_time_queue)
{
   // Update MG1 Model Parameters
   _sigma_service_time_square += square(service_time);
   _sigma_service_time += service_time;
   _num_arrivals ++;
   _newest_arrival_time = getMax<UInt64>(_newest_arrival_time, pkt_time + waiting_time_queue + service_time);
}
