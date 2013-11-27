#include "carbon_user.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

// Suggested DVFS domain configuration:
// <1.0, CORE, L1_ICACHE, L1_DCACHE> <1.0, L2_CACHE, DIRECTORY> <1.0, NETWORK_MEMORY, NETWORK_USER>

void doMemoryWork()
{
   //int ARRAY_SIZE = 8192*64;
   int ARRAY_SIZE = 512*64;
   int NUM_ITERATIONS = 100;
   int8_t array [ARRAY_SIZE];
   for (int i=0; i<ARRAY_SIZE; i+=64){
      array[i] = i;
   }

   CarbonEnableModels();

   for (int j= 0; j<NUM_ITERATIONS; j++){
      for (int i=0; i<ARRAY_SIZE; i+=64){
         array[i] += i;
      }
   }

   CarbonDisableModels();

}

void doComputeWork()
{
   volatile int i;
   volatile double j;
   for (i=0; i<10000; i++){
      j = j*(i+2);
   }
}

int main()
{
   double frequency = 0;
   double voltage = 0;
   int rc;
   module_t domain1 = CarbonGetDVFSDomain(L2_CACHE);
   module_t domain2 = CarbonGetDVFSDomain(NETWORK_MEMORY);

   frequency = 1.0;

   rc = CarbonSetDVFSAllTiles(domain1, &frequency, AUTO);
   if (rc != 0)
   {
      fprintf(stderr, "*ERROR* Setting frequency for Domain(%u), Return-Code(%i)\n", domain1, rc);
      exit(EXIT_FAILURE);
   }

   rc = CarbonSetDVFSAllTiles(domain2, &frequency, AUTO);
   if (rc != 0)
   {
      fprintf(stderr, "*ERROR* Setting frequency for Domain(%u), Return-Code(%i)\n", domain2, rc);
      exit(EXIT_FAILURE);
   }

   doComputeWork();
   doMemoryWork();
}
