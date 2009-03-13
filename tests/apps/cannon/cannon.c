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

#include "carbon_user.h"

//#define DEBUG 1
// #define USE_INT 1
// #define USE_FLOAT_INTERNAL_REPRESENTATION 1
//#define MATRIX_DEBUG 1
//#define ADDRESS_DEBUG 1

#ifdef USE_INT
typedef SInt32 DType;
#else
typedef float DType;
#endif

// Declare global variables
DType **a;
DType **b;
DType **c;
int blockSize, sqrtNumProcs;
pthread_mutex_t write_lock;

#ifdef DEBUG
pthread_mutex_t lock;
#else

#ifdef MATRIX_DEBUG
pthread_mutex_t lock;
#else 

#ifdef ADDRESS_DEBUG
pthread_mutex_t lock;
#endif

#endif

#endif

// Debug
long numSendCalls;
long numReceiveCalls;
long *numThreadSends;
long *numThreadReceives;

// Function executed by each thread
void* cannon(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim();

    fprintf(stderr, "Start of main...\n");
   unsigned int matSize, num_threads;

   if (argc != 5)
   {
      printf("Invalid command line options. The correct format is:\n");
      printf("cannon -m num_of_threads -s size_of_square_matrix\n");
      exit(EXIT_FAILURE);
   }
   else
   {
      if ((strcmp(argv[1], "-m\0") == 0) && (strcmp(argv[3], "-s\0") == 0))
      {
         num_threads = atoi(argv[2]);
         matSize = atoi(argv[4]);
      }
      else if ((strcmp(argv[1], "-s\0") == 0) && (strcmp(argv[3], "-m\0") == 0))
      {
         num_threads = atoi(argv[4]);
         matSize = atoi(argv[2]);
      }
      else
      {
         printf("Invalid command line options. The correct format is:\n");
         printf("cannon -m num_of_threads -s size_of_square_matrix\n");
         exit(EXIT_FAILURE);
      }
   }

   // Declare threads and related variables
   carbon_thread_t threads[num_threads];

#ifdef DEBUG
   printf("Initializing global variables\n");
#endif
   // Initialize global variables

   // Debug
   numSendCalls = 0;
   numThreadSends = (long*)malloc(num_threads*sizeof(long));
   for (unsigned int i = 0; i < num_threads; i++) numThreadSends[i] = 0;

   // Debug
   numReceiveCalls = 0;
   numThreadReceives = (long*)malloc(num_threads*sizeof(long));
   for (unsigned int i = 0; i < num_threads; i++) numThreadReceives[i] = 0;

   // Initialize a
   a = (DType**)malloc(matSize*sizeof(DType*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      a[i] = (DType*)malloc(matSize*sizeof(DType));
      for (unsigned int j = 0; j < matSize; j++) a[i][j] = 2;
   }

   // Initialize b
   b = (DType**)malloc(matSize*sizeof(DType*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      b[i] = (DType*)malloc(matSize*sizeof(DType));
      for (unsigned int j = 0; j < matSize; j++) b[i][j] = 3;
   }

   // Initialize c
   c = (DType**)malloc(matSize*sizeof(DType*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      c[i] = (DType*)malloc(matSize*sizeof(DType));
      for (unsigned int j = 0; j < matSize; j++) c[i][j] = 0;
   }

   // FIXME: we get a compiler warning here because the output of
   // sqrt is being converted to an int.  We really should be doing
   // some sanity checking of num_threads and matSize to be sure
   // these calculations go alright.
   double tmp = num_threads;
   sqrtNumProcs = (float) sqrt(tmp);
   blockSize = matSize / sqrtNumProcs;

#ifdef DEBUG
   printf("Initializing thread structures\n");
   pthread_mutex_init(&lock, NULL);
#else
#ifdef MATRIX_DEBUG
   printf("Initializing thread structures\n");
   pthread_mutex_init(&lock, NULL);
#else
#ifdef ADDRESS_DEBUG
   printf("Initializing thread structures\n");
   pthread_mutex_init(&lock, NULL);
#endif
#endif
#endif

   // Initialize threads and related variables
   pthread_mutex_init(&write_lock, NULL);

   for (unsigned int i = 0; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(cannon, (void *) i);

   // Wait for all threads to complete
   for (unsigned int i = 0; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   // Print out the result matrix
   printf("\n\n\n");
   printf("-----------------\n");
   printf("c = \n");
   for (unsigned int i = 0; i < matSize; i++)
   {
      for (unsigned int j = 0; j < matSize; j++)
      {
#ifdef USE_INT
         printf("%d ", c[i][j]);
#else
#ifdef USE_FLOAT_INTERNAL_REPRESENTATION
         printf("0x%x ", *((UInt32*) &c[i][j]));
#else
         printf("%f ", c[i][j]);
#endif
#endif
      }
      printf("\n");
   }

   // Debug
   printf("Number of sends = %ld\n", numSendCalls);
   printf("Number of receives = %ld\n", numReceiveCalls);

   CarbonStopSim();
} // main ends

void* cannon(void *threadid)
{

   // Declare local variables
   int tid;
   int upProc, downProc, rightProc, leftProc;
   DType **aBlock, **bBlock, **cBlock;
   int i, j;
   int ai, aj, bi, bj;
   CAPI_return_t rtnVal;

#ifdef DEBUG
   printf("Starting thread %d\n", (int)threadid);
#endif

   rtnVal = CAPI_Initialize((unsigned int)threadid);
   sleep(5);

   // Initialize local variables

   CAPI_rank(&tid);
   tid = (int)threadid;

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
   aBlock = (DType**)malloc(blockSize*sizeof(DType*));
   for (int x = 0; x < blockSize; x++)
   {
      aBlock[x] = (DType*)malloc(blockSize*sizeof(DType));
      for (int y = 0; y < blockSize; y++) aBlock[x][y] = a[ai + x][aj + y];
   }

   // populate bBlock
   bBlock = (DType**)malloc(blockSize*sizeof(DType*));
   for (int x = 0; x < blockSize; x++)
   {
      bBlock[x] = (DType*)malloc(blockSize*sizeof(DType));
      for (int y = 0; y < blockSize; y++) bBlock[x][y] = b[bi + x][bj + y];
   }

   // Initialize cBlock
   cBlock = (DType**)malloc(blockSize*sizeof(DType*));
   for (int x = 0; x < blockSize; x++)
   {
      cBlock[x] = (DType*)malloc(blockSize*sizeof(DType));
      for (int y = 0; y < blockSize; y++) cBlock[x][y] = 0;
   }


#ifdef ADDRESS_DEBUG
   if (tid == 0)
   {
      for (SInt32 x = 0; x < blockSize; x++)
      {
         for (SInt32 y = 0; y < blockSize; y++)
         {
            pthread_mutex_lock(&lock);
            fprintf (stderr, "c[%d][%d] = %p\n", x, y, &c[x][y]);
            pthread_mutex_unlock(&lock);
         }
      }
   }
   else
   {
      sleep(20);
   }
#endif

   for (int iter = 0; iter < sqrtNumProcs; iter++) // for loop begins
   {
      for (int x = 0; x < blockSize; x++) {
         for (int y = 0; y < blockSize; y++) {
            for (int z = 0; z < blockSize; z++) {

#ifdef MATRIX_DEBUG
               pthread_mutex_lock(&lock);
               fprintf (stderr, "Before MULT operation: ");
#ifdef USE_INT
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %d, bBlock[%d][%d] = %d, cBlock[%d][%d] = %d\n", tid, iter, x, z, aBlock[x][z], z, y, bBlock[z][y], x, y, cBlock[x][y]);
#else

#ifdef USE_FLOAT_INTERNAL_REPRESENTATION
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %u, bBlock[%d][%d] = %u, cBlock[%d][%d] = %u\n", tid, iter, x, z, *((UInt32*) &aBlock[x][z]), z, y, *((UInt32*) &bBlock[z][y]), x, y, *((UInt32*) &cBlock[x][y]) );
#else
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %f, bBlock[%d][%d] = %f, cBlock[%d][%d] = %f\n", tid, iter, x, z, aBlock[x][z], z, y, bBlock[z][y], x, y, cBlock[x][y]);
#endif

#endif
               pthread_mutex_unlock(&lock);
#endif

               cBlock[x][y] += aBlock[x][z] * bBlock[z][y];

#ifdef MATRIX_DEBUG
               pthread_mutex_lock(&lock);
               fprintf (stderr, "After MULT operation: ");
#ifdef USE_INT
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %d, bBlock[%d][%d] = %d, cBlock[%d][%d] = %d\n", tid, iter, x, z, aBlock[x][z], z, y, bBlock[z][y], x, y, cBlock[x][y]);
#else

#ifdef USE_FLOAT_INTERNAL_REPRESENTATION
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %u, bBlock[%d][%d] = %u, cBlock[%d][%d] = %u\n", tid, iter, x, z, *((UInt32*) &aBlock[x][z]), z, y, *((UInt32*) &bBlock[z][y]), x, y, *((UInt32*) &cBlock[x][y]) );
#else
               fprintf (stderr, "tid #%d: iter #%d: aBlock[%d][%d] = %f, bBlock[%d][%d] = %f, cBlock[%d][%d] = %f\n", tid, iter, x, z, aBlock[x][z], z, y, bBlock[z][y], x, y, cBlock[x][y]);
#endif

#endif
               pthread_mutex_unlock(&lock);
#endif
            }
         }
      }

      if (iter < sqrtNumProcs - 1) // if block begins
      {
         // Send aBlock left

         for (int x = 0; x < blockSize; x++)
            for (int y = 0; y < blockSize; y++)
            {
#ifdef DEBUG
               pthread_mutex_lock(&lock);
               printf("tid # %d sending to tid # %d\n", tid, leftProc);
               pthread_mutex_unlock(&lock);
#endif
               CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)leftProc, (char*)&aBlock[x][y], sizeof(DType));
               // Debug
               pthread_mutex_lock(&write_lock);
               numSendCalls++;
               numThreadSends[tid]++;
               pthread_mutex_unlock(&write_lock);
            }

         // Send bBlock up
         for (int x= 0; x < blockSize; x++)
            for (int y = 0; y < blockSize; y++)
            {
#ifdef DEBUG
               pthread_mutex_lock(&lock);
               printf("tid # %d sending to tid # %d\n", tid, upProc);
               pthread_mutex_unlock(&lock);
#endif
               CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)upProc, (char*)&bBlock[x][y], sizeof(DType));
               // Debug
               pthread_mutex_lock(&write_lock);
               numSendCalls++;
               numThreadSends[tid]++;
               pthread_mutex_unlock(&write_lock);
            }


         // Receive aBlock from right
         for (int x = 0; x < blockSize; x++)
            for (int y = 0; y < blockSize; y++)
            {
#ifdef DEBUG
               pthread_mutex_lock(&lock);
               printf("tid # %d receiving from tid # %d\n", tid, rightProc);
               pthread_mutex_unlock(&lock);
#endif
               CAPI_message_receive_w((CAPI_endpoint_t)rightProc, (CAPI_endpoint_t)tid, (char*)&aBlock[x][y], sizeof(DType));
               // Debug
               pthread_mutex_lock(&write_lock);
               numReceiveCalls++;
               numThreadReceives[tid]++;
               pthread_mutex_unlock(&write_lock);
            }

         // Receive bBlock from below
         for (int x = 0; x < blockSize; x++)
            for (int y = 0; y < blockSize; y++)
            {
#ifdef DEBUG
               pthread_mutex_lock(&lock);
               printf("tid # %d receiving from tid # %d\n", tid, downProc);
               pthread_mutex_unlock(&lock);
#endif
               CAPI_message_receive_w((CAPI_endpoint_t)downProc, (CAPI_endpoint_t)tid, (char*)&bBlock[x][y], sizeof(DType));
               // Debug
               pthread_mutex_lock(&write_lock);
               numReceiveCalls++;
               numThreadReceives[tid]++;
               pthread_mutex_unlock(&write_lock);
            }

      } // if block ends

   } // for loop ends


   // Update c
   for (int x = 0; x < blockSize; x++)
      for (int y = 0; y < blockSize; y++) c[ai + x][bj + y] = cBlock[x][y];

   return NULL;
}

