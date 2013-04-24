#include "carbon_user.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int main()
{
   double frequency = 0;
   double voltage = 0;
   int rc;

   // local core
   rc = CarbonGetDVFS(0, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 2.0;
   rc = CarbonSetDVFS(0, CORE, &frequency, AUTO);
   assert(rc == 0);

   rc = CarbonGetDVFS(0, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 1.0;
   rc = CarbonSetDVFS(0, CORE, &frequency, HOLD);
   assert(rc == 0);

   rc = CarbonGetDVFS(0, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   // remote core
   rc = CarbonGetDVFS(10, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 2.0;
   rc = CarbonSetDVFS(10, CORE, &frequency, AUTO);
   assert(rc == 0);

   rc = CarbonGetDVFS(10, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

   frequency = 1.0;
   rc = CarbonSetDVFS(10, CORE, &frequency, HOLD);
   assert(rc == 0);

   rc = CarbonGetDVFS(10, CORE, &frequency, &voltage);
   assert(rc == 0);
   printf("frequency(%g), voltage(%g)\n", frequency, voltage);

}
