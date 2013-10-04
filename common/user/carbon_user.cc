#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "tile_manager.h"
#include "tile.h"
#include "core_model.h"
#include "config_file.hpp"
#include "handle_args.h"
#include "carbon_user.h"
#include "thread_support_private.h"

static config::ConfigFile cfg;

tile_id_t CarbonGetTileId()
{
   return Sim()->getTileManager()->getCurrentTileID();
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
      Sim()->getTileManager()->initializeThread(Tile::getMainCoreId(0));
  
      if (Sim()->getConfig()->getSimulationMode() == Config::FULL)
         CarbonSpawnThreadSpawner();

      LOG_PRINT("Returning to main()...");
      return 0;
   }
   else
   {
      LOG_PRINT("Replacing main()...");

      assert(Sim()->getConfig()->getSimulationMode() == Config::FULL);
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
   Core* core = Sim()->getTileManager()->getCurrentCore();
   UInt64 time = core->getModel()->getCurrTime().toNanosec();

   return time;
}
