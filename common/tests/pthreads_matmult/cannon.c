/****************************************************
 * This is a first pass implementation of Cannon's 
 * algorithm for matrix multiplication. The two input
 * matrices are hard-coded into the program and we 
 * make unabashed use of the shared memory nature of 
 * pthreads to get around programming tasks that are
 * not central to the problem at hand
 *
 *                          - Harshad Kasture
 *                          01/06/2008
 ****************************************************/

#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "capi.h"


//#define DEBUG 1

// Declare global variables
float **a, **b, **c;
int blockSize, sqrtNumProcs;
pthread_mutex_t write_lock;

#ifdef DEBUG
pthread_mutex_t lock;
#endif

// Debug
long numSendCalls;
long numReceiveCalls;
long *numThreadSends;
long *numThreadReceives;

// Function executed by each thread
void* cannon(void * threadid);

int main(int argc, char* argv[]){ // main begins
	
	// Read in the command line arguments
	
	unsigned int matSize, numThreads;
	pthread_t* threads;
	pthread_attr_t attr;

#ifdef DEBUG
	printf("This is the function main()\n");
#endif

	if(argc != 5){
	   printf("Invalid command line options. The correct format is:\n"); 
	   printf("cannon -m num_of_threads -s size_of_square_matrix\n");
	   exit(EXIT_FAILURE);
	}
	else{
		if((strcmp(argv[1], "-m\0") == 0) && (strcmp(argv[3], "-s\0") == 0)){
			numThreads = atoi(argv[2]);
			matSize = atoi(argv[4]);
		}
		else if((strcmp(argv[1], "-s\0") == 0) && (strcmp(argv[3], "-m\0") == 0)){
			numThreads = atoi(argv[4]);
			matSize = atoi(argv[2]);
		}
		else{
		   printf("Invalid command line options. The correct format is:\n"); 
		   printf("cannon -m num_of_threads -s size_of_square_matrix\n");
		   exit(EXIT_FAILURE);
		}
	}

#ifdef DEBUG
	printf("Number of threads: %d     Matrix size: %d\n", numThreads, matSize);
#endif

	// Declare threads and related variables
	threads = (pthread_t*)malloc(numThreads*sizeof(pthread_t));
	
#ifdef DEBUG
	printf("Initializing global variables\n");
#endif
	// Initialize global variables

	// Debug
	numSendCalls = 0;
	numThreadSends = (long*)malloc(numThreads*sizeof(long));
	for(unsigned int i = 0; i < numThreads; i++) numThreadSends[i] = 0;

	// Debug
	numReceiveCalls = 0;
	numThreadReceives = (long*)malloc(numThreads*sizeof(long));
	for(unsigned int i = 0; i < numThreads; i++) numThreadReceives[i] = 0;

	// Initialize a
	a = (float**)malloc(matSize*sizeof(float*));
	for(unsigned int i = 0; i < matSize; i++){
	   a[i] = (float*)malloc(matSize*sizeof(float));
		for(unsigned int j = 0; j < matSize; j++) a[i][j] = 2;
	}

	// Initialize b
	b = (float**)malloc(matSize*sizeof(float*));
	for(unsigned int i = 0; i < matSize; i++){
	   b[i] = (float*)malloc(matSize*sizeof(float));
		for(unsigned int j = 0; j < matSize; j++) b[i][j] = 3;
	}

	// Initialize c
	c = (float**)malloc(matSize*sizeof(float*));
	for(unsigned int i = 0; i < matSize; i++){
	   c[i] = (float*)malloc(matSize*sizeof(float));
		for(unsigned int j = 0; j < matSize; j++) c[i][j] = 0;
	}
	
	// FIXME: we get a compiler warning here because the output of
	// sqrt is being converted to an int.  We really should be doing
	// some sanity checking of numThreads and matSize to be sure
	// these calculations go alright.
        double tmp = numThreads;
	sqrtNumProcs = (float) sqrt(tmp);
	blockSize = matSize / sqrtNumProcs;

#ifdef DEBUG
	printf("Initializing thread structures\n");
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);
#ifdef DEBUG
	printf("Spawning threads\n");
#endif
	for(unsigned int i = 0; i < numThreads; i++) {
	   int err = pthread_create(&threads[i], &attr, cannon, (void *) i);
	   if (err != 0) {
	      printf("  ERROR spawning thread %d!  Error code: %s\n",
		     i, strerror(err));
	   }
#ifdef DEBUG
	   else { printf("  Spawned thread %d\n", i); }
#endif
	}

	// Wait for all threads to complete
	for(unsigned int i = 0; i < numThreads; i++) pthread_join(threads[i], NULL);

	// Print out the result matrix
	printf("c = \n");
	for(unsigned int i = 0; i < matSize; i++){
		for(unsigned int j = 0; j < matSize; j++){
		   printf("%f ", c[i][j]);
		}
		printf("\n");
	}

	// Debug
	printf("Number of sends = %ld\n", numSendCalls);
	printf("Number of receives = %ld\n", numReceiveCalls);

	pthread_exit(NULL);
} // main ends

void* cannon(void *threadid){

	// Declare local variables
	int tid;
	int upProc, downProc, rightProc, leftProc;
	float **aBlock, **bBlock, **cBlock;
	int i, j;
	int ai, aj, bi, bj;
	CAPI_return_t rtnVal;

#ifdef DEBUG
	printf("Starting thread %d\n", (unsigned int)threadid);
#endif

	rtnVal = CAPI_Initialize((int)threadid);

	// Initialize local variables

	CAPI_rank(&tid);

	// Convert 1-D rank to 2-D rank
	i = tid / sqrtNumProcs;
	j = tid % sqrtNumProcs;

	upProc = (i == 0)?(sqrtNumProcs - 1):(i - 1);
	upProc = (upProc * sqrtNumProcs) + j;

	downProc = (i == (sqrtNumProcs - 1))?0:(i + 1);
	downProc = (downProc * sqrtNumProcs) + j;

	rightProc = (j == (sqrtNumProcs - 1))?0:(j + 1);
	rightProc = (i * sqrtNumProcs) + rightProc;

	leftProc = (j == 0)?(sqrtNumProcs - 1):(j - 1);
	leftProc = (i * sqrtNumProcs) + leftProc;

	ai = i * blockSize;
	aj = ((i + j) < sqrtNumProcs)?(i + j):(i + j - sqrtNumProcs);
	aj = aj * blockSize;

	bi = ((i + j) < sqrtNumProcs)?(i + j):(i + j - sqrtNumProcs);
	bi = bi * blockSize;
	bj = j * blockSize;
	
	// populate aBlock
	aBlock = (float**)malloc(blockSize*sizeof(float*));
	for(int x = 0; x < blockSize; x++){
		aBlock[x] = (float*)malloc(blockSize*sizeof(float));
		for(int y = 0; y < blockSize; y++) aBlock[x][y] = a[ai + x][aj + y];
	}

	// populate bBlock
	bBlock = (float**)malloc(blockSize*sizeof(float*));
	for(int x = 0; x < blockSize; x++){
		bBlock[x] = (float*)malloc(blockSize*sizeof(float));
		for(int y = 0; y < blockSize; y++) bBlock[x][y] = b[bi + x][bj + y];
	}

	// Initialize cBlock
	cBlock = (float**)malloc(blockSize*sizeof(float*));
	for(int x = 0; x < blockSize; x++){
		cBlock[x] = (float*)malloc(blockSize*sizeof(float));
		for(int y = 0; y < blockSize; y++) cBlock[x][y] = 0;
	}

	for(int iter = 0; iter < sqrtNumProcs; iter++){ // for loop begins
		for(int x = 0; x < blockSize; x++)
			for(int y = 0; y < blockSize; y++)
				for(int z = 0; z < blockSize; z++) cBlock[x][y] += aBlock[x][z] * bBlock[z][y];

		if(iter < sqrtNumProcs - 1){ // if block begins
			// Send aBlock left

			for(int x = 0; x < blockSize; x++)
				for(int y = 0; y < blockSize; y++) {
#ifdef DEBUG
					pthread_mutex_lock(&lock);
					printf("tid # %d sending to tid # %d\n", tid, leftProc);
					pthread_mutex_unlock(&lock);
#endif
					CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)leftProc, (char*)&aBlock[x][y], sizeof(float));
					// Debug
					pthread_mutex_lock(&write_lock);
					numSendCalls++;
					numThreadSends[tid]++;
					pthread_mutex_unlock(&write_lock);
				}

			// Send bBlock up
			for(int x= 0; x < blockSize; x++)
				for(int y = 0; y < blockSize; y++) {
#ifdef DEBUG
					pthread_mutex_lock(&lock);
					printf("tid # %d sending to tid # %d\n", tid, upProc);
					pthread_mutex_unlock(&lock);
#endif
					CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)upProc, (char*)&bBlock[x][y], sizeof(float));
					// Debug
					pthread_mutex_lock(&write_lock);
					numSendCalls++;
					numThreadSends[tid]++;
					pthread_mutex_unlock(&write_lock);
				}


			// Receive aBlock from right
			for(int x = 0; x < blockSize; x++)
				for(int y = 0; y < blockSize; y++) {
#ifdef DEBUG
					pthread_mutex_lock(&lock);
					printf("tid # %d receiving from tid # %d\n", tid, rightProc);
					pthread_mutex_unlock(&lock);
#endif
					CAPI_message_receive_w((CAPI_endpoint_t)rightProc, (CAPI_endpoint_t)tid, (char*)&aBlock[x][y], sizeof(float));
					// Debug
					pthread_mutex_lock(&write_lock);
					numReceiveCalls++;
					numThreadReceives[tid]++;
					pthread_mutex_unlock(&write_lock);
				}

			// Receive bBlock from below
			for(int x = 0; x < blockSize; x++)
				for(int y = 0; y < blockSize; y++) {
#ifdef DEBUG
					pthread_mutex_lock(&lock);
					printf("tid # %d receiving from tid # %d\n", tid, downProc);
					pthread_mutex_unlock(&lock);
#endif
					CAPI_message_receive_w((CAPI_endpoint_t)downProc, (CAPI_endpoint_t)tid, (char*)&bBlock[x][y], sizeof(float));
					// Debug
					pthread_mutex_lock(&write_lock);
					numReceiveCalls++;
					numThreadReceives[tid]++;
					pthread_mutex_unlock(&write_lock);
				}

		} // if block ends

	} // for loop ends


	// Update c
	for(int x = 0; x < blockSize; x++)
		for(int y = 0; y < blockSize; y++) c[ai + x][bj + y] = cBlock[x][y];

	pthread_exit(NULL);
}

