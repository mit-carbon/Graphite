#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include "carbon_user.h"

int num_threads;
int num_iterations;
double min_frequency;
double max_frequency;
pthread_barrier_t global_barrier;

void *do_work(void*)
{
   SInt32 tile_id = CarbonGetTileId();
   SInt32 remote_tile_id;

   // Wait for all threads to be spawned
   pthread_barrier_wait(&global_barrier);

   // Initialize the random number generator
   struct drand48_data rand_buf;
   srand48_r(tile_id, &rand_buf);   
	for (int i=0; i<num_iterations; i++)
   {
      double res;
      drand48_r(&rand_buf, &res);
      double frequency = min_frequency + (max_frequency - min_frequency) * res;

      drand48_r(&rand_buf, &res);
      remote_tile_id = (SInt32) (num_threads * res);
      
      int a = (int) floor(frequency);
      int b = (int) floor( (frequency - floor(frequency)) * 100 );
      printf("Setting frequency to (%i.%i) on tile (%i) from tile (%i) iteration (%i)\n", a, b,remote_tile_id, tile_id, i);

      CarbonSetDVFS(remote_tile_id, TILE, &frequency, AUTO);

      // Do some work
		for (int j = 0; j < 10000; j++);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
   if (argc != 5)
   {
      fprintf(stderr, "[Usage] ./frequency_scaling_remote <Number of Threads> <Number of Frequency Changes Per Thread> <Min Frequency (in GHz)> <Max Frequency (in GHz)>\n");
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
