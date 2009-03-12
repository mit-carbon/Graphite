#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
#include "config_file.hpp"

#include "carbon_user.h"
#include "thread_support_private.h"

config::ConfigFile cfg;

int CarbonStartSim()
{
   cfg.load("carbon_sim.cfg");

   Simulator::setConfig(&cfg);

   Simulator::allocate();
   Sim()->start();

   // First start the thread spawner
   CarbonSpawnThreadSpawner();

   if (Config::getSingleton()->getCurrentProcessNum() == 0)
   {
      // Main process
      Sim()->getCoreManager()->initializeThread(0);

      return 0;
   }
   else
   {
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
