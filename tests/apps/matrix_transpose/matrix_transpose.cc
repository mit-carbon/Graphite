#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carbon_user.h"

volatile SInt64** initial_matrix;
volatile SInt64** final_matrix;
SInt32 matrix_size;
SInt32 num_threads;

carbon_barrier_t barrier;

void* thread_func(void* threadid);
void parse_args(int argc, char *argv[]);

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   parse_args(argc, argv);

   printf("Starting Matrix Transpose: Num Threads(%i), Matrix Size(%i)\n", num_threads, matrix_size);

   // Allocate the matrix
   initial_matrix = (volatile SInt64**) new SInt64*[matrix_size];
   final_matrix = (volatile SInt64**) new SInt64*[matrix_size];
   for (SInt32 i = 0; i < matrix_size; i++)
   {
      initial_matrix[i] = new SInt64[matrix_size];
      final_matrix[i] = new SInt64[matrix_size];
   }

   // Initialize the matrix
   for (SInt32 i = 0; i < matrix_size; i++)
      for (SInt32 j = 0; j < matrix_size; j++)
         initial_matrix[i][j] = i*j;

   // Initialize barrier
   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads-1];

   // Spawn the threads
   for (SInt32 i = 0; i < num_threads-1; i++)
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   thread_func((void*) (num_threads-1));

   // Join all the threads
   for (SInt32 i = 0; i < num_threads-1; i++)
      CarbonJoinThread(tid_list[i]);

   CarbonStopSim();

   /*
   printf("Initial Matrix\n");
   for (SInt32 i = 0; i < matrix_size; i++)
   {
      for (SInt32 j = 0; j < matrix_size; j++)
         printf("%lli\t", (long long int) initial_matrix[i][j]);
      printf("\n");
   }
   
   printf("Final Matrix\n");
   for (SInt32 i = 0; i < matrix_size; i++)
   {
      for (SInt32 j = 0; j < matrix_size; j++)
         printf("%lli\t", (long long int) final_matrix[i][j]);
      printf("\n");
   }
   */
   
   printf("Matrix Transpose: Completed Successfully\n");
   return 0;
}

void* thread_func(void* threadid)
{
   long tid = (long) threadid;
   SInt32 num_rows_per_thread = matrix_size / num_threads;
   
   assert(matrix_size % num_threads == 0);

   SInt64 initial_sum = 0;
   __attribute((__unused__)) SInt64 junk = 0;

   // Each processor reads certain rows of the initial and final matrices into the local cache
   for (SInt32 i = (tid * num_rows_per_thread); i < ((tid+1) * num_rows_per_thread); i++)
   {
      for (SInt32 j = 0; j < matrix_size; j++)
      {
         initial_sum += initial_matrix[i][j];
         junk += final_matrix[i][j];
      }
   }

   // Invert the matrix
   // Each processor writes certain columns of the matrix
   // Lots of invalidates goes forth
   for (SInt32 i = 0; i < matrix_size; i++)
   {
      for (SInt32 j = (tid * num_rows_per_thread); j < ((tid+1) * num_rows_per_thread); j++)
      {
         final_matrix[i][j] = initial_matrix[j][i];
      }
   }

   CarbonBarrierWait(&barrier);
   
   SInt64 final_sum = 0;
   // Each processor reads certain rows of the matrix again
   for (SInt32 i = (tid * num_rows_per_thread); i < ((tid+1) * num_rows_per_thread); i++)
   {
      for (SInt32 j = 0; j < matrix_size; j++)
      {
         final_sum += final_matrix[i][j];
      }
   }

   assert(initial_sum == final_sum);
   /*
   printf("Thread(%i): Initial Sum(%lli), Final Sum(%lli)\n", tid, (long long int) initial_sum, (long long int) final_sum);
   */

   return (void*) NULL;
}

void parse_args(int argc, char* argv[])
{
   assert(argc >= 5);
   for (SInt32 i = 1; i < 5; i += 2)
   {
      if (strcmp(argv[i], "-t") == 0)
         num_threads = atoi(argv[i+1]);
      else if (strcmp(argv[i], "-m") == 0)
         matrix_size = atoi(argv[i+1]);
      else
      {
         printf("Unrecognized switch(%s)\n", argv[i]);
         exit(-1);
      }
   }      
}
