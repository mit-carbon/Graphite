#include "simulator.h"
#include "config.h"
#include "queue_model_basic.h"
#include "utils.h"
#include "log.h"

QueueModelBasic::QueueModelBasic()
   : QueueModel(BASIC)
   , _queue_time(0)
   , _moving_average(NULL)
{
   bool moving_avg_enabled = false;
   UInt32 moving_avg_window_size = 0;
   std::string moving_avg_type_str;
   try
   {
      moving_avg_enabled = Sim()->getCfg()->getBool("queue_model/basic/moving_avg_enabled", false);
      moving_avg_window_size = Sim()->getCfg()->getInt("queue_model/basic/moving_avg_window_size", 1);
      moving_avg_type_str = Sim()->getCfg()->getString("queue_model/basic/moving_avg_type", "none");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read queue_model/basic parameters from the cfg file");
   }

   if (moving_avg_enabled)
   {
      _moving_average = MovingAverage<UInt64>::createAvgType(moving_avg_type_str, moving_avg_window_size);
   }
}

QueueModelBasic::~QueueModelBasic() 
{}

UInt64
QueueModelBasic::computeQueueDelay(UInt64 pkt_time, UInt64 processing_time, tile_id_t requester)
{
   // Compute the moving average here
   UInt64 ref_time;
   if (_moving_average)
   {
      ref_time = _moving_average->compute(pkt_time);
   }
   else
   {
      ref_time = pkt_time;
   }

   UInt64 queue_delay = (_queue_time > ref_time) ? (_queue_time - ref_time) : 0;
  
   LOG_PRINT("Pkt Time(%llu), Ref Time(%llu), Queue Time(%llu), Queue Delay(%llu), Requester(%i)",
             pkt_time, ref_time, _queue_time, queue_delay, requester);

   // Update the Queue Time
   _queue_time = getMax<UInt64>(_queue_time, ref_time) + processing_time;

   // Update Utilization Counters
   updateQueueUtilizationCounters(ref_time, processing_time, queue_delay);

   return queue_delay;
}
