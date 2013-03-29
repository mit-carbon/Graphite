#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "carbon_user.h"

int num_threads;
int num_iterations;
float min_frequency;
float max_frequency;
pthread_barrier_t global_barrier;

void *do_work(void*)
{
   SInt32 tile_id = CarbonGetTileId();

   // Initialize the random number generator
   struct drand48_data rand_buf;
   srand48_r(tile_id, &rand_buf);   

   // change frequency
   double res;
   drand48_r(&rand_buf, &res);
   float frequency = min_frequency + (max_frequency - min_frequency) * res;

      
   int a = (int) floor(frequency);
   int b = (int) floor( (frequency - floor(frequency)) * 100 );
   printf("Setting frequency to (%i.%i) on tile (%i)\n", a, b, tile_id);

   if (tile_id != 0)
      CarbonSetTileFrequency(&frequency);

   // Wait for all threads to be spawned
   pthread_barrier_wait(&global_barrier);

   float r = 0;
	for (int i=0; i<num_iterations; i++)
   {
      // Do some work
		for (int j = 0; j < 1000; j++){
         drand48_r(&rand_buf, &res);
         r = res; 
      };
	}

	return NULL;
}

int main(int argc, char *argv[])
{
   if (argc != 5)
   {
      fprintf(stderr, "[Usage] ./frequency_scaling_simple <Number of Threads> <Number of Frequency Changes Per Thread> <Min Frequency (in GHz)> <Max Frequency (in GHz)>\n");
      exit(EXIT_FAILURE);
   }

   num_threads = atoi(argv[1]);
   num_iterations = atoi(argv[2]);
   min_frequency = atof(argv[3]);
   max_frequency = atof(argv[4]);

   // Initialize barrier
   pthread_barrier_init(&global_barrier, NULL, num_threads);

	pthread_t worker[num_threads];

	for (int i=1; i<num_threads; i++)
   {
		int ret = pthread_create(&worker[i], NULL, do_work, NULL);
      if (ret != 0)
      {
         fprintf(stderr, "*Error* pthread_create FAILED\n");
         exit(EXIT_FAILURE);
      }
	}
   do_work(NULL);
	
	for (int i=1; i<num_threads; i++)
   {
		pthread_join(worker[i], NULL);
	}

   // Destroy barrier
   pthread_barrier_destroy(&global_barrier);

	return 0;
}
