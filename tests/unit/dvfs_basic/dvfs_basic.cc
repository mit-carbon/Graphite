#include "carbon_user.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Suggested DVFS domain configuration:
// <1.0, CORE, L1_ICACHE, L1_DCACHE, L2_CACHE, DIRECTORY> <1.0, NETWORK_MEMORY, NETWORK_USER>

int main()
{
   double frequency = 0;
   double voltage = 0;
   int rc;

   // local core
   module_t domain = CarbonGetDVFSDomain(CORE);
   rc = CarbonGetDVFS(0, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 2.0;
   rc = CarbonSetDVFSAllTiles(domain, &frequency, AUTO);
   assert(rc == 0);

   rc = CarbonGetDVFS(0, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 1.0;
   rc = CarbonSetDVFSAllTiles(domain, &frequency, HOLD);
   assert(rc == 0);

   rc = CarbonGetDVFS(0, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   // remote core
   rc = CarbonGetDVFS(1, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 2.0;
   rc = CarbonSetDVFSAllTiles(domain, &frequency, AUTO);
   assert(rc == 0);

   rc = CarbonGetDVFS(1, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 1.0;
   rc = CarbonSetDVFSAllTiles(domain, &frequency, HOLD);
   assert(rc == 0);

   rc = CarbonGetDVFS(1, domain, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   return 0;
}
