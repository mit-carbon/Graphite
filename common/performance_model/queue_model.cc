#include "queue_model.h"
#include "simulator.h"
#include "config.h"
#include "queue_model_basic.h"
#include "queue_model_history_list.h"
#include "log.h"

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
   else
   {
      LOG_PRINT_ERROR("Unrecognized Queue Model Type(%s)", model_type.c_str());
      return (QueueModel*) NULL;
   }
}

