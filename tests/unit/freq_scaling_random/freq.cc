#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "carbon_user.h"


void *do_work(void *id)
{

	int my_id = *((int *) id);
   float frequency;
   int a, b;
	
	for (int i=0; i<1000; i++){

      frequency = ((float)(rand()%100) * 0.02) + 1.0; //  1GHz <= frequency < 3GHz
      a = (int) frequency;
      b = (int)((frequency - a)*10000);
      printf("Setting frequency to (%i.%i) on core (%i) iteration (%i)\n",a,b,my_id,i);

      CarbonSetTileFrequency(&frequency);

		int j = 0;
		while (j < 10000){ j++; }
	}

	return NULL;
}

int main()
{
	int n_workers = 60;
	int tid[n_workers];
	pthread_t worker[n_workers];

   srand((unsigned)time(NULL));

	for (int i=0; i<n_workers; i++){
		tid[i] = *(new int(i));
		pthread_create(&worker[i], NULL, do_work, &tid[i]);
	}
	
	for (int i=0; i<n_workers; i++){
		pthread_join(worker[i], NULL);
	}

	return 0;

}
