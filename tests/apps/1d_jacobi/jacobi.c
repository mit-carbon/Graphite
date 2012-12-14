#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <limits.h>

#include "carbon_user.h"
#include "capi.h"
#include "sync_api.h"

#define NUM_ITERS 100
#define SWAP(a,b,t)  (((t) = (a)), ((a) = (b)), ((b) = (t)))

// Global Variables
float* _old_array;
float* _new_array;
SInt32 _num_threads;
SInt32 _size;

carbon_barrier_t jacobi_barrier;

// Function Declarations
void* threadMain(void*);
void printArray(SInt32);

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);
   if (argc != 3)
   {
      fprintf(stderr, "[Usage]: ./jacobi <Number of Threads> <Number of Array Elements per Thread>\n");
      exit(EXIT_FAILURE);
   }

   _num_threads = atoi(argv[1]);
   int num_array_elements_per_core = atoi(argv[2]);
   
   _size = _num_threads * num_array_elements_per_core;

   fprintf (stderr, "\nStarting (jacobi)\n");
   fprintf (stderr, "Number of Cores = %d\n", _num_threads);
   fprintf (stderr, "Size of the array = %d\n", _size);

   _old_array = (float *) malloc((_size+2) * sizeof(float));
   _new_array = (float *) malloc((_size+2) * sizeof(float));

   for (SInt32 i = 0; i < _size+2; i++)
   {
      _old_array[i] = 0;
      _new_array[i] = 0;
   }
   _old_array[0] = INT_MAX;
   _new_array[0] = INT_MAX;

   // Initialize the barrier
   CarbonBarrierInit(&jacobi_barrier, _num_threads);
   
   carbon_thread_t threads[_num_threads];

   for (SInt32 i = 1; i < _num_threads; i++)
   {
      threads[i] = CarbonSpawnThread(threadMain, (void*) i);
      if (threads[i] < 0)
      {
         fprintf(stderr, "ERROR spawning thread %d!\n", i);         
         exit(EXIT_FAILURE);
      }
   }
   threadMain((void*) 0);

   for (SInt32 i = 1; i < _num_threads; i++)
   {
      CarbonJoinThread(threads[i]);
   }

   printArray(NUM_ITERS);

   fprintf(stderr, "Done (jacobi). Exiting...\n\n");
   CarbonStopSim();
   return (0);
}

void* threadMain(void *threadid)
{
   SInt32 tid = (SInt32) threadid;

   CarbonBarrierWait(&jacobi_barrier);

   SInt32 start_index = ((_size/_num_threads) * tid) + 1;
   SInt32 end_index = ((_size/_num_threads) * (tid + 1)) + 1;

   for (SInt32 k = 0; k < NUM_ITERS; k++)
   {
      for (SInt32 i = start_index; i < end_index; i++)
      {
         _new_array[i] = (_old_array[i-1] + _old_array[i+1]) / 2;
      }

      CarbonBarrierWait(&jacobi_barrier);

      if (tid == 0)
      {
         float* temp;
         SWAP(_old_array, _new_array, temp);
      }

      CarbonBarrierWait(&jacobi_barrier);

   }

   return NULL;
}

void printArray(SInt32 iter)
{

   fprintf(stderr, "Contents of Array after iteration: %d\n", iter);
   for (SInt32 i = 0; i < _size+2; i++)
      fprintf(stderr, "%f, ", _old_array[i]);
   fprintf(stderr, "\n");

}
