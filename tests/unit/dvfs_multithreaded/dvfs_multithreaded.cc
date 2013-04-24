#include "carbon_user.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

int num_threads;
int num_iterations;
int iteration_size;
int array_size;
int cache_line_size;
int8_t* array;
double memory_low_frequency;
double memory_high_frequency;
double compute_low_frequency;
double compute_high_frequency;
pthread_barrier_t global_barrier;

void doMemoryWork(int current_iteration)
{
   volatile int a;
   for (int i=0; i<iteration_size; i++){
      a += array[(current_iteration*iteration_size + i)*cache_line_size];
   }
}

void doComputeWork(int current_iteration)
{
   volatile int i;
   volatile double j;
   int num_compute_iterations = 10;
   for (int k=0; k<num_compute_iterations; k++){
      for (i=0; i<iteration_size; i++){
         int a = array[(current_iteration*iteration_size + i)*cache_line_size];
         j = j*a;
      }
   }
}

void* doWork(void*)
{
   SInt32 tile_id = CarbonGetTileId();
   double frequency;
   int rc;

   pthread_barrier_wait(&global_barrier);

   for (int i= 0; i<num_iterations; i++){

      //if (tile_id == 0) printf("Iteration: %i (memory)\n", i);

      // set memory dvfs
      rc = CarbonSetDVFS(tile_id, CORE | L1_ICACHE | L1_DCACHE, &memory_low_frequency, AUTO);
      assert(rc == 0);
      rc = CarbonSetDVFS(tile_id, L2_CACHE | NETWORK_MEMORY, &memory_high_frequency, AUTO);
      assert(rc == 0);

      //doMemoryWork(i);

      pthread_barrier_wait(&global_barrier);

      //if (tile_id == 0) printf("Iteration: %i (compute)\n", i);

      // set compute dvfs
      rc = CarbonSetDVFS(tile_id, CORE | L1_ICACHE | L1_DCACHE, &compute_high_frequency, AUTO);
      assert(rc == 0);
      rc = CarbonSetDVFS(tile_id, L2_CACHE | NETWORK_MEMORY, &compute_low_frequency, AUTO);
      assert(rc == 0);

      //doComputeWork(i);

      // Wait for all threads to be spawned
      pthread_barrier_wait(&global_barrier);
   }

   return NULL;
}

int main(int argc, char *argv[])
{

   if (argc != 9)
   {
      fprintf(stderr,"Error: invalid number of arguments\n");
      exit(EXIT_FAILURE);
   }

   num_threads = atoi(argv[1]);
   num_iterations = atoi(argv[2]);
   iteration_size = atoi(argv[3]);
   cache_line_size = atoi(argv[4]);
   memory_low_frequency = atof(argv[5]);
   memory_high_frequency = atof(argv[6]);
   compute_low_frequency = atof(argv[7]);
   compute_high_frequency = atof(argv[8]);
   array_size = num_iterations*iteration_size*cache_line_size;
   array = (int8_t*) malloc(sizeof(int8_t)*array_size);

	pthread_t worker[num_threads];

   // Initialize barrier
   pthread_barrier_init(&global_barrier, NULL, num_threads);

   CarbonEnableModels();

	for (int i=1; i<num_threads; i++)
   {
		int ret = pthread_create(&worker[i], NULL, doWork, NULL);
      if (ret != 0)
      {
         fprintf(stderr, "*Error* pthread_create FAILED\n");
         exit(EXIT_FAILURE);
      }
	}
   doWork(NULL);


	for (int i=1; i<num_threads; i++)
   {
		pthread_join(worker[i], NULL);
	}

   CarbonDisableModels();

   // Destroy barrier
   pthread_barrier_destroy(&global_barrier);

   free(array);
   
   return 0;

}
