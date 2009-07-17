#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "carbon_user.h"
#include "capi.h"
#include "sync_api.h"

// #define DEBUG 1
#define NUM_ITERS 5
#define SWAP(a,b,t)  (((t) = (a)), ((a) = (b)), ((b) = (t)))

// Global Variables
float** g_old_array;
float** g_new_array;

SInt32 num_cores;
SInt32 _2d_array_size_per_core;
SInt32 _2d_array_size;

carbon_barrier_t jacobi_barrier;

#ifdef DEBUG
pthread_mutex_t print_lock;
#endif

void* threadMain(void*);
void printElements(SInt32);

int main (int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

/*    if (argc != 3) */
/*    { */
/*       fprintf(stderr, "[Usage]: ./jacobi <Number of Threads> <2-d array size per Thread>\n"); */
/*       exit(-1); */
/*    } */

   printf("Starting 2d-jacobi relaxation..\n");

   num_cores = atoi(argv[1]);
   _2d_array_size_per_core = atoi(argv[2]);
   _2d_array_size = sqrt(num_cores) * _2d_array_size_per_core;

   // Allocate the storage space
   g_old_array = (float**) malloc((_2d_array_size+2) * sizeof(float*));
   g_new_array = (float**) malloc((_2d_array_size+2) * sizeof(float*));
   for (SInt32 i = 0; i < _2d_array_size+2; i++)
   {
      g_old_array[i] = (float*) malloc((_2d_array_size+2) * sizeof(float));
      g_new_array[i] = (float*) malloc((_2d_array_size+2) * sizeof(float));

      for (SInt32 j = 0; j < _2d_array_size+2; j++)
      {
         g_old_array[i][j] = 0.0;
         g_new_array[i][j] = 0.0;
      }
   }

   // Initialize to the correct value
   for (SInt32 i = 1; i < _2d_array_size+1; i++)
   {
      for (SInt32 j = 1; j < _2d_array_size+1; j++)
      {
         g_old_array[i][j] = i*j;
         g_new_array[i][j] = 0.0;
      }
   }

   printElements(0);

   CarbonBarrierInit(&jacobi_barrier, num_cores);

#ifdef DEBUG
   pthread_mutex_init(&print_lock, NULL);
#endif

   carbon_thread_t threads[num_cores];

   for (SInt32 i = 0; i < num_cores; i++)
   {
      threads[i] = CarbonSpawnThread(threadMain, (void*) i);
      if (threads[i] < 0) {
         fprintf(stderr, "ERROR spawning thread %d!\n", i);         
         exit(-1);
      }

#ifdef DEBUG
      pthread_mutex_lock(&print_lock);
      fprintf(stderr, "Created Thread: %i\n", i);
      pthread_mutex_unlock(&print_lock);
#endif
   }

   for (SInt32 i = 0; i < num_cores; i++)
   {
      CarbonJoinThread(threads[i]);

#ifdef DEBUG
      pthread_mutex_lock(&print_lock);
      fprintf(stderr, "Joined Thread: %i\n", i);
      pthread_mutex_unlock(&print_lock);
#endif
   }

#ifndef DEBUG
   printElements(NUM_ITERS);
#endif

   printf("Done. Exiting..\n");

   CarbonStopSim();
   return 0;
}

void* threadMain(void* threadid)
{
   SInt32 tid = (SInt32) threadid;

#ifdef DEBUG
   pthread_mutex_lock(&print_lock);
   fprintf(stderr, "Starting Thread: %i\n", tid);
   pthread_mutex_unlock(&print_lock);
#endif

   SInt32 x_coord = tid % ((SInt32) sqrt(num_cores));
   SInt32 y_coord = tid / ((SInt32) sqrt(num_cores));

   SInt32 start_index_x = (x_coord * _2d_array_size_per_core) + 1;
   SInt32 end_index_x = ((x_coord+1) * _2d_array_size_per_core) + 1;
   SInt32 start_index_y = (y_coord * _2d_array_size_per_core) + 1;
   SInt32 end_index_y = ((y_coord+1) * _2d_array_size_per_core) + 1;

   for (SInt32 k = 0; k < NUM_ITERS; k++)
   {
      for (SInt32 i = start_index_x; i < end_index_x; i++)
      {
         for (SInt32 j = start_index_y; j < end_index_y; j++)
         {
            g_new_array[i][j] = (g_old_array[i-1][j] + g_old_array[i+1][j] + g_old_array[i][j-1] + g_old_array[i][j+1]) / 4;
         }
      }

      CarbonBarrierWait(&jacobi_barrier);

      // Swap the 2 arrays
      if (tid == 0)
      {
         float **temp;
         SWAP(g_old_array, g_new_array, temp);

#ifdef DEBUG
         printElements(k+1);
#endif
      }

      CarbonBarrierWait(&jacobi_barrier);
   }

   return 0;
}

void printElements(SInt32 iter)
{
   printf("\n2d array elements after iteration: %i\n", iter);
   printf("-------------------------------------\n");
   for (SInt32 i = 0; i < _2d_array_size+2; i++)
   {
      for (SInt32 j = 0; j < _2d_array_size+2; j++)
      {
         printf("%f\t", g_old_array[i][j]);
      }
      printf("\n");
   }
}

