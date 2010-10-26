#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
#include "clock_converter.h"
#include "config_file.hpp"
#include "handle_args.h"
#include "fxsupport.h"

#include "carbon_user.h"
#include "thread_support_private.h"

static config::ConfigFile cfg;

core_id_t CarbonGetCoreId()
{
   return Sim()->getCoreManager()->getCurrentCoreID();
}

int CarbonStartSim(int argc, char **argv)
{
   string_vec args;

   // Set the default config path if it isn't 
   // overwritten on the command line.
   std::string config_path = "carbon_sim.cfg";

   // Parse the arguments that are relative
   // to the config file changes as well
   // as extracting the config_path
   parse_args(args, config_path, argc, argv);

   cfg.load(config_path);
   handle_args(args, cfg);

   Simulator::setConfig(&cfg);

   Simulator::allocate();
   Sim()->start();

   if (Config::getSingleton()->getCurrentProcessNum() == 0)
   {
      // Main process
      Sim()->getCoreManager()->initializeThread(0);
   
      CarbonSpawnThreadSpawner();

      LOG_PRINT("Returning to main()...");
      return 0;
   }
   else
   {
      LOG_PRINT("Replacing main()...");

      CarbonThreadSpawner (NULL);

      // Not main process
      while (!Sim()->finished())
         usleep(100);

      CarbonStopSim();

      // Otherwise we will run main ...
      exit(0);
   }
}

void CarbonStopSim()
{
   Simulator::release();
}

UInt64 CarbonGetTime()
{
   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   Tile* core = Sim()->getCoreManager()->getCurrentCore();
   UInt64 time = convertCycleCount(core->getPerformanceModel()->getCycleCount(), \
         core->getPerformanceModel()->getFrequency(), 1.0);

   return time;
}
