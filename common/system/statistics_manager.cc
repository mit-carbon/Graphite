#include "statistics_manager.h"
#include "simulator.h"
#include "config.h"
#include "memory_manager.h"
#include "network.h"
#include "utils.h"
#include "log.h"

StatisticsManager::StatisticsManager()
{
   string enabled_statistics_line;
   try
   {
      enabled_statistics_line = Sim()->getCfg()->getString("statistics_trace/statistics");
      _sampling_interval = Sim()->getCfg()->getInt("statistics_trace/sampling_interval");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read statistics_trace information from cfg file");
   }

   vector<string> enabled_statistics;
   splitIntoTokens(enabled_statistics_line, enabled_statistics, ", ");
   
   for (vector<string>::iterator it = enabled_statistics.begin(); it != enabled_statistics.end(); it ++)
   {
      StatisticType type = parseType(*it);
      _statistic_enabled[type] = true;
   }
  
   // Open trace files
   openTraceFiles(); 
}

StatisticsManager::~StatisticsManager()
{
   // Close trace files
   closeTraceFiles();
}

void
StatisticsManager::openTraceFiles()
{
   for (SInt32 i = 0; i < NUM_STATISTIC_TYPES; i++)
   {
      if (_statistic_enabled[i])
      {
         switch (i)
         {
         case CACHE_LINE_REPLICATION:
            MemoryManager::openCacheLineReplicationTraceFiles();
            break;

         case NETWORK_UTILIZATION:
            Network::openUtilizationTraceFiles();
            break;

         default:
            LOG_PRINT_ERROR("Unrecognized Statistic Type(%i)", i);
            break;
         }
      }
   }
}

void
StatisticsManager::closeTraceFiles()
{
   for (SInt32 i = 0; i < NUM_STATISTIC_TYPES; i++)
   {
      if (_statistic_enabled[i])
      {
         switch (i)
         {
         case CACHE_LINE_REPLICATION:
            MemoryManager::closeCacheLineReplicationTraceFiles();
            break;

         case NETWORK_UTILIZATION:
            Network::closeUtilizationTraceFiles();
            break;

         default:
            LOG_PRINT_ERROR("Unrecognized Statistic Type(%i)", i);
            break;
         }
      }
   }
}

void
StatisticsManager::outputPeriodicSummary()
{
   for (SInt32 i = 0; i < NUM_STATISTIC_TYPES; i++)
   {
      if (_statistic_enabled[i])
      {
         switch (i)
         {
         case CACHE_LINE_REPLICATION:
            MemoryManager::outputCacheLineReplicationSummary();
            break;

         case NETWORK_UTILIZATION:
            Network::outputUtilizationSummary();
            break;

         default:
            LOG_PRINT_ERROR("Unrecognized Statistic Type(%i)", i);
            break;
         }
      }
   }
}
   
StatisticsManager::StatisticType
StatisticsManager::parseType(string type)
{
   if (type == "cache_line_replication")
      return CACHE_LINE_REPLICATION;
   else if (type == "network_utilization")
      return NETWORK_UTILIZATION;
   else
      return NUM_STATISTIC_TYPES;
}
