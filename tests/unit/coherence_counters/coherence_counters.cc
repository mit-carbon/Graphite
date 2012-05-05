#include <cstdio>
#include "carbon_user.h"

#define NUM_ITERATIONS  10
#define ARRAY_SIZE 65536
int main(int argc, char* argv[])
{
   UInt64 total = 0;
   UInt64 a[ARRAY_SIZE];
   for (UInt32 j = 0; j < ARRAY_SIZE; j++)
      a[j] = j;

   CarbonEnableModels();
   for (UInt32 i = 0; i < NUM_ITERATIONS; i++)
   {
      for (UInt32 j = 0; j < ARRAY_SIZE; j++)
         total += a[j];
   }
   CarbonDisableModels();

   printf("res = %llu\n", (long long unsigned int) total);
   return 0;
}
