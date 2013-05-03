#include "carbon_user.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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

   frequency = 1.0;
   rc = CarbonSetDVFSAllTiles(L2_CACHE | NETWORK_MEMORY, &frequency, AUTO);
   assert(rc == 0);

   doComputeWork();
   doMemoryWork();
}
