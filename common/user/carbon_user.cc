#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include "simulator.h"
#include "thread_manager.h"
#include "core_manager.h"
#include "core.h"
#include "config_file.hpp"
#include "handle_args.h"

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

// This is the simple wrapper function that gets called if we don't replace the
// create_pthread call. This is simply used to identify the cases where we actually
// want to use the pthread library (within the simulator) versus when we want to
// user our thread spawning mechanism (within the application).
int create_pthread(pthread_t * thread, pthread_attr_t * attr, void * (*start_routine)(void *), void * arg)
{
   return pthread_create(thread,attr,start_routine,arg);
}

