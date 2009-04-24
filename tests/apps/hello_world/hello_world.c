#include <stdio.h>
#include <unistd.h>

#include "carbon_user.h"

int main(int argc, char **argv)
{
   CarbonStartSim(argc, argv);
   printf("Hello, world!\n");
   CarbonStopSim();
   return 0;
}
