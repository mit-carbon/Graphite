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

#include <iostream>
#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include "capi.h"

using namespace std;


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

	if(argc != 5){
		cout << "Invalid command line options. The correct format is:" << endl;
		cout << "cannon -m num_of_threads -s size_of_square_matrix" << endl;
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
			cout << "Invalid command line options. The correct format is:" << endl;
			cout << "cannon -m num_of_threads -s size_of_square_matrix" << endl;
			exit(EXIT_FAILURE);
		}
	}

	// Declare threads and related variables
	pthread_t threads[numThreads];
	pthread_attr_t attr;
	
#ifdef DEBUG
	cout << "This is the function main()" << endl;
#endif
	// Initialize global variables

	// Debug
	numSendCalls = 0;
	numThreadSends = new long [numThreads];
	for(unsigned int i = 0; i < numThreads; i++) numThreadSends[i] = 0;

	// Debug
	numReceiveCalls = 0;
	numThreadReceives = new long [numThreads];
	for(unsigned int i = 0; i < numThreads; i++) numThreadReceives[i] = 0;

	// Initialize a
	a = (float**) new float*[matSize];
	for(unsigned int i = 0; i < matSize; i++){
		a[i] = (float*) new float[matSize];
		for(unsigned int j = 0; j < matSize; j++) a[i][j] = 1;
	}

	// Initialize b
	b = (float**) new float*[matSize];
	for(unsigned int i = 0; i < matSize; i++){
		b[i] = (float*) new float[matSize];
		for(unsigned int j = 0; j < matSize; j++) b[i][j] = 1;
	}

	// Initialize c
	c = (float**) new float*[matSize];
	for(unsigned int i = 0; i < matSize; i++){
		c[i] = (float*) new float[matSize];
		for(unsigned int j = 0; j < matSize; j++) c[i][j] = 0;
	}
	
        double tmp = numThreads;
	sqrtNumProcs = sqrt(tmp);
	blockSize = matSize / sqrtNumProcs;

#ifdef DEBUG
	cout << "Initializing thread structures" << endl;
	pthread_mutex_init(&lock, NULL);
#endif

	// Initialize threads and related variables
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	pthread_mutex_init(&write_lock, NULL);
#ifdef DEBUG
	cout << "Spawning threads" << endl;
#endif
	for(unsigned int i = 0; i < numThreads; i++) pthread_create(&threads[i], &attr, cannon, (void *) i);

	// Wait for all threads to complete
	for(unsigned int i = 0; i < numThreads; i++) pthread_join(threads[i], NULL);

	// Print out the result matrix
	cout << "c = " << endl;
	for(unsigned int i = 0; i < matSize; i++){
		for(unsigned int j = 0; j < matSize; j++){
			cout << c[i][j] << " ";
		}
		cout << endl;
	}

	// Debug
	cout << "Number of sends = " << numSendCalls << endl;
	cout << "Number of receives = " << numReceiveCalls << endl;

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

	rtnVal = CAPI_Initialize();

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
	aBlock = (float**)new float*[blockSize];
	for(int x = 0; x < blockSize; x++){
		aBlock[x] = (float*)new float[blockSize];
		for(int y = 0; y < blockSize; y++) aBlock[x][y] = a[ai + x][aj + y];
	}

	// populate bBlock
	bBlock = (float**)new float*[blockSize];
	for(int x = 0; x < blockSize; x++){
		bBlock[x] = (float*)new float[blockSize];
		for(int y = 0; y < blockSize; y++) bBlock[x][y] = b[bi + x][bj + y];
	}

	// Initialize cBlock
	cBlock = (float**)new float*[blockSize];
	for(int x = 0; x < blockSize; x++){
		cBlock[x] = (float*)new float[blockSize];
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
					cout << "tid #" << tid << " sending to tid #" << leftProc << endl;
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
					cout << "tid #" << tid << " sending to tid #" << upProc << endl;
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
					cout << "tid #" << tid << " receiving from tid #" << rightProc << endl;
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
					cout << "tid #" << tid << " receiving from tid #" << downProc << endl;
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

