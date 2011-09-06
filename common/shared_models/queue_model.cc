#include "queue_model.h"
#include "simulator.h"
#include "config.h"
#include "queue_model_basic.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"
#include "log.h"

QueueModel::QueueModel(Type type)
   : _type(type)
{
   initializeQueueUtilizationCounters();
}

QueueModel::~QueueModel()
{}

QueueModel*
QueueModel::create(std::string model_type, UInt64 min_processing_time)
{
   if (model_type == "basic")
   {
      return new QueueModelBasic();
   }
   else if (model_type == "history_list")
   {
      return new QueueModelHistoryList(min_processing_time);
   }
   else if (model_type == "history_tree")
   {
      return new QueueModelHistoryTree(min_processing_time);
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Queue Model Type(%s)", model_type.c_str());
      return (QueueModel*) NULL;
   }
}

void
QueueModel::initializeQueueUtilizationCounters()
{
   _total_utilized_cycles = 0;
   _last_request_time = 0;
   _total_requests = 0;
}

void
QueueModel::updateQueueUtilizationCounters(UInt64 request_time, UInt64 processing_time, UInt64 queue_delay)
{
   _total_utilized_cycles += processing_time;
   _last_request_time = max<UInt64>(_last_request_time, request_time + queue_delay + processing_time);
   _total_requests ++;
}

float
QueueModel::getQueueUtilization()
{
   // Approximate total cycles as _last_request_time + _last_service_time
   UInt64 total_cycles = _last_request_time;
   return (total_cycles > 0) ? (((float) _total_utilized_cycles) / total_cycles) : 0.0;
}
