#include <stdlib.h>
#include <stdio.h>

#include "omp.h"
#include "carbon_user.h"

int main( int argc, char *argv[])
{
   // Enable power & performance models
   CarbonEnableModels();

   int nthreads, tid;

   #pragma omp parallel private(tid,nthreads) num_threads(2)
   {
      tid = omp_get_thread_num();
      printf("Hello World from thread %d\n", tid);
      fflush(NULL);

      if (tid == 0)
      {
         nthreads = omp_get_num_threads();
         printf("Number of threads %d\n", nthreads);
         fflush(NULL);
      }
   }

   // Disable power & performance models
   CarbonDisableModels();

   return 0;
}
