#include "perf_counter_support.h"
#include "tile.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

void CarbonEnableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      fprintf(stderr, "[[Graphite]] --> [ Enabling Performance and Power Models ]\n");
      // Enable the models of the cores in the current process
      Simulator::enablePerformanceModelsInCurrentProcess();
   }
}

void CarbonDisableModels()
{
   if (Sim()->getCfg()->getBool("general/trigger_models_within_application", false))
   {
      fprintf(stderr, "[[Graphite]] --> [ Disabling Performance and Power Models ]\n");
      // Disable performance models of cores in this process
      Simulator::disablePerformanceModelsInCurrentProcess();
   }
} 
