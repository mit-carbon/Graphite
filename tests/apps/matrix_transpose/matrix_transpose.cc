#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "carbon_user.h"

int64_t** initial_matrix;
int64_t** final_matrix;
int32_t matrix_size;
int32_t num_threads;

carbon_barrier_t barrier;

void* thread_func(void* threadid);
void parse_args(int argc, char *argv[]);

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   parse_args(argc, argv);

   printf("Starting Matrix Transpose: Num Threads(%i), Matrix Size(%i)\n", num_threads, matrix_size);

   // Allocate the matrix
   initial_matrix = (int64_t**) new int64_t*[matrix_size];
   final_matrix = (int64_t**) new int64_t*[matrix_size];
   for (int32_t i = 0; i < matrix_size; i++)
   {
      initial_matrix[i] = new int64_t[matrix_size];
      final_matrix[i] = new int64_t[matrix_size];
   }

   // Initialize the matrix
   for (int32_t i = 0; i < matrix_size; i++)
      for (int32_t j = 0; j < matrix_size; j++)
         initial_matrix[i][j] = (i+2)*j;

   // Initialize barrier
   CarbonBarrierInit(&barrier, num_threads);

   carbon_thread_t tid_list[num_threads-1];

   // Spawn the threads
   for (int32_t i = 0; i < num_threads-1; i++)
      tid_list[i] = CarbonSpawnThread(thread_func, (void*) i);
   thread_func((void*) (num_threads-1));

   // Join all the threads
   for (int32_t i = 0; i < num_threads-1; i++)
      CarbonJoinThread(tid_list[i]);


   printf("Initial Matrix\n");
   for (int32_t i = 0; i < matrix_size; i++)
   {
      for (int32_t j = 0; j < matrix_size; j++)
         printf("%lli\t", (long long int) initial_matrix[i][j]);
      printf("\n");
   }
   
   printf("Final Matrix\n");
   for (int32_t i = 0; i < matrix_size; i++)
   {
      for (int32_t j = 0; j < matrix_size; j++)
         printf("%lli\t", (long long int) final_matrix[i][j]);
      printf("\n");
   }
   
   printf("Matrix Transpose: Completed Successfully\n");
   
   CarbonStopSim();
   return 0;
}

void* thread_func(void* threadid)
{
   long tid = (long) threadid;
   int32_t num_rows_per_thread = matrix_size / num_threads;
   
   assert(matrix_size % num_threads == 0);

   int64_t initial_sum = 0;
   __attribute__((unused)) int64_t junk = 0;

   // Each processor reads certain rows of the initial and final matrices into the local cache
   for (int32_t i = (tid * num_rows_per_thread); i < ((tid+1) * num_rows_per_thread); i++)
   {
      for (int32_t j = 0; j < matrix_size; j++)
      {
         initial_sum += initial_matrix[i][j];
         junk += final_matrix[i][j];
      }
   }

   // Invert the matrix
   // Each processor writes certain columns of the matrix
   // Lots of invalidates goes forth
   for (int32_t i = 0; i < matrix_size; i++)
   {
      for (int32_t j = (tid * num_rows_per_thread); j < ((tid+1) * num_rows_per_thread); j++)
      {
         final_matrix[i][j] = initial_matrix[j][i];
      }
   }

   CarbonBarrierWait(&barrier);
   
   int64_t final_sum = 0;
   // Each processor reads certain rows of the matrix again
   for (int32_t i = (tid * num_rows_per_thread); i < ((tid+1) * num_rows_per_thread); i++)
   {
      for (int32_t j = 0; j < matrix_size; j++)
      {
         final_sum += final_matrix[j][i];
      }
   }

   assert(initial_sum == final_sum);

   return (void*) NULL;
}

void parse_args(int argc, char* argv[])
{
   assert(argc >= 5);
   for (int32_t i = 1; i < 5; i += 2)
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
