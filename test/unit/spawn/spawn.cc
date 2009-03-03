// This unit test will test the spawning mechanisms of the simulator

#include <stdio.h>
#include "simulator.h"
#include "config_file.hpp"

int main(int argc, char **argv)
{
   config::ConfigFile cfg;

   cfg.Load("../../../carbon_sim.cfg");
   Simulator::setConfig(&cfg);

   fprintf(stderr, "Allocating a simulator.\n");
   Simulator::allocate();
   fprintf(stderr, "Starting the simulator.\n");
   Sim()->start();
   fprintf(stderr, "Releasing the simulator.\n");
   Simulator::release();
   fprintf(stderr, "done.");
   return 0;
}

