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
      bool moving_avg_enabled = Sim()->getCfg()->getBool("queue_model/basic/moving_avg_enabled", false);
      UInt32 moving_avg_window_size = Sim()->getCfg()->getInt("queue_model/basic/moving_avg_window_size", 1);
      std::string moving_avg_type = Sim()->getCfg()->getString("queue_model/basic/moving_avg_type", "none");
      return new QueueModelBasic(moving_avg_enabled, moving_avg_window_size, moving_avg_type);
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

