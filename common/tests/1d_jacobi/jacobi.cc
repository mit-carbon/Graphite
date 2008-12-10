#include <iostream>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "capi.h"
#include "user_api.h"


#define DEBUG 1

using namespace std;

typedef int Int32;
typedef unsigned int UInt32;

#define NUM_ITERS 10
#define SWAP(a,b,t)  (((t) = (a)), ((a) = (b)), ((b) = (t)))

// Global Variables
Int32* g_old_array;
Int32* g_new_array;
Int32 g_num_cores;
Int32 g_size;

carbon_barrier_t jacobi_barrier;

pthread_mutex_t print_lock;

// Function Declarations
void* threadMain (void*);
void printArray (Int32);

void wait_some()
{
   Int32 j = 0;
   for (UInt32 i = 0; i < 20000; i++)
   {
      j += i;
      asm volatile("nop");
   }
}

int main (int argc, char *argv[]) {

	if (argc != 3) {
		cerr << "[Usage]: ./jacobi <Number of Cores> <Size of Array>\n";
		exit(-1);
	}

	carbonInit();

	g_num_cores = atoi(argv[1]);
	g_size = atoi (argv[2]);

	g_old_array = (Int32 *) malloc ((g_size+2) * sizeof(Int32));
	g_new_array = (Int32 *) malloc ((g_size+2) * sizeof(Int32));

	cerr << "g_old_array = 0x" << hex << (UInt32) g_old_array << endl;
	cerr << "g_new_array = 0x" << hex << (UInt32) g_new_array << dec << endl;

	for (Int32 i = 0; i < g_size+2; i++) {
		g_old_array[i] = 0;
		g_new_array[i] = 0;
	}
	g_old_array[0] = 32768;
	g_new_array[0] = 32768;
	
	pthread_mutex_init (&print_lock, NULL);

	pthread_t threads[g_num_cores];
	pthread_attr_t attr;

	pthread_attr_init (&attr);
	pthread_attr_setdetachstate (&attr, PTHREAD_CREATE_JOINABLE);

#ifdef DEBUG
	pthread_mutex_lock (&print_lock);
	cerr << "Creating Threads\n";
	pthread_mutex_unlock (&print_lock);
#endif

	for (Int32 i = 0; i < g_num_cores; i++) {
#ifdef DEBUG
		pthread_mutex_lock (&print_lock);
		cerr << "Creating Thread: " << i << endl;
		pthread_mutex_unlock (&print_lock);
#endif
		pthread_create (&threads[i], &attr, threadMain, (void*) i);
	}
	
	for (Int32 i = 0; i < g_num_cores; i++) {
		pthread_join (threads[i], NULL);
	}

	carbonFinish();

	return (0);
}

void* threadMain (void *threadid) {

	Int32 tid = (Int32) threadid;

	CAPI_Initialize (tid);

	if (tid == 0) {
#ifdef DEBUG
		pthread_mutex_lock (&print_lock);
		cerr << "Starting threadMain - Thread 0\n";
		printArray (-1);
		pthread_mutex_unlock (&print_lock);
#endif

		barrierInit (&jacobi_barrier, g_num_cores);
	}
	else {
		sleep(1);
	}

	if (tid != 0) {
		wait_some();
	}

#ifdef DEBUG
	pthread_mutex_lock (&print_lock);
	cerr << "Thread [" << tid << "]: Waiting for barrier\n";
	pthread_mutex_unlock (&print_lock);
#endif

	barrierWait (&jacobi_barrier);

#ifdef DEBUG
	pthread_mutex_lock (&print_lock);
	cerr << "Thread [" << tid << "]: Finished barrier\n";
	pthread_mutex_unlock (&print_lock);
#endif

	Int32 start_index = ((g_size/g_num_cores) * tid) + 1;
	Int32 end_index = ((g_size/g_num_cores) * (tid + 1)) + 1;

#ifdef DEBUG
	pthread_mutex_lock (&print_lock);
	cerr << "Thread [" << tid << "]: Start Index = " << start_index << endl;
	cerr << "Thread [" << tid << "]: End Index = " << end_index << endl;
	pthread_mutex_unlock (&print_lock);
#endif

	for (Int32 k = 0; k < NUM_ITERS; k++) {
		for (Int32 i = start_index; i < end_index; i++) {
			g_new_array[i] = (g_old_array[i-1] + g_old_array[i+1]) / 2;
		}

		barrierWait (&jacobi_barrier);

		if (tid == 0) {

			Int32* temp;
			SWAP (g_old_array, g_new_array, temp);

#ifdef DEBUG
			pthread_mutex_lock (&print_lock);
			printArray (k);
			pthread_mutex_unlock (&print_lock);
#endif
		}
		
		barrierWait (&jacobi_barrier);

	}

	pthread_exit(NULL);

}

void printArray(Int32 iter) {

	cerr << "Contents of Array after iteration: " << iter << endl;
	for (Int32 i = 0; i < g_size+2; i++) {
		cerr << g_old_array[i] << ", ";
	}
	cerr << endl << endl;

}
