/****************************************************
 * This is a test that will test the realloc call   *
 ****************************************************/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <assert.h>

#include "carbon_user.h"

// Functions executed by threads
void* test_realloc(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);

   const unsigned int num_threads = 10;
   carbon_thread_t threads[num_threads];

   for(unsigned int i = 0; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(test_realloc, (void *) i);

   for(unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   printf("Finished running realloc test!\n");

   CarbonStopSim();

   return 0;
} // main ends


void* test_realloc(void *threadid)
{
	int init_size = 5;
	long rank = (long)(threadid);
	char *myString = (char *)malloc(init_size * sizeof(char));

	for(int i = 2; i < 100; i++) {
		int num_chars = init_size * i;
		myString = realloc(myString, num_chars * sizeof(char));
		for(int j=0; j<num_chars; j++) {
			myString[j] = 'j';
		}
		
		for(int k=0; k<num_chars; k++) {
			assert(myString[k] == 'j');
		}
	}

	printf("[%ld]\tdone realloc\n", rank);
	free(myString);
	
	return NULL;
}

