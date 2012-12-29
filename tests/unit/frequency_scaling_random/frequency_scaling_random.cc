#include <pthread.h>
#include <cstdio>
#include <cstdlib>
#include "carbon_user.h"

int num_threads;
int num_iterations;
float min_frequency;
float max_frequency;

void *do_work(void*)
{
   SInt32 tile_id = CarbonGetTileId();

   // Initialize the random number generator
   struct drand48_data rand_buf;
   srand48_r(tile_id, &rand_buf);   
	for (int i=0; i<num_iterations; i++)
   {
      double res;
      drand48_r(&rand_buf, &res);
      float frequency = min_frequency + (max_frequency - min_frequency) * res;
      printf("Setting frequency to (%.2f) on tile (%i) iteration (%i)\n", frequency, tile_id, i);

      CarbonSetTileFrequency(&frequency);

      // Do some work
		for (int j = 0; j < 10000; j++);
	}

	return NULL;
}

int main(int argc, char *argv[])
{
   if (argc != 5)
   {
      fprintf(stderr, "[Usage] ./frequency_scaling_random <Number of Threads> <Number of Frequency Changes Per Thread> <Min Frequency (in GHz)> <Max Frequency (in GHz)>\n");
      exit(EXIT_FAILURE);
   }

   num_threads = atoi(argv[1]);
   num_iterations = atoi(argv[2]);
   min_frequency = atof(argv[3]);
   max_frequency = atof(argv[4]);

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

	return 0;
}
