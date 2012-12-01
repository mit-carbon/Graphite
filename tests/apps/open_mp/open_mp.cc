#include <stdlib.h>
#include <stdio.h>

#include "omp.h"

int main( int argc, char *argv[])
{
   int nthreads, tid;

   omp_set_num_threads(2);

   #pragma omp parallel private(tid,nthreads) num_threads(2)
   {
      tid = omp_get_thread_num();
      printf("Hello World from thread %d\n", tid);

      if (tid == 0)
      {
         nthreads = omp_get_num_threads();
         printf("Number of threads %d\n", nthreads);
      }
   }

   return 0;
}
