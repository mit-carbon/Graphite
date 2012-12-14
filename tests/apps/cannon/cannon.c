/********************************************************
 * This message passing only implementation of Cannon's *
 * algorithm for matrix multiplication. The two input   *
 * matrices are hard-coded into the program.            *
 *            - (edited from Harshad Kasture's version) *
 *                          - Charles Gruenwald         *
 *                          12/12/2008                  *
 *******************************************************/

#include <pthread.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <assert.h>

#include "carbon_user.h"

// #define DEBUG 1
// #define SEQUENTIAL 1

#define ITERATIONS 1

int do_cannon(int argc, char* argv[]);
void* cannon(void* threadid);

void spawner_wait_ack(int id, unsigned int num_threads);
void worker_send_ack(int tid, unsigned int num_threads);

void worker_wait_go(int tid, unsigned int num_threads);
void spawner_send_go(int tid, unsigned int num_threads);

void debug_printf(carbon_mutex_t* lock, const char * fmt, ...)
{
#ifdef DEBUG
   va_list ap;
   va_start(ap, fmt);
   
   CarbonMutexLock(lock);
   vfprintf(stderr, fmt, ap);
   CarbonMutexUnlock(lock);
#endif
}

void printErrorAndExit()
{
   fprintf(stderr, "\n[Usage]: cannon -t <num_of_threads> -m <square_matrix_size>\n");
   fprintf(stderr, "<num_of_threads> must be a perfect square\n");
   fprintf(stderr, "<square_matrix_size> should be a multiple of sqrt(<num_of_threads>)\n\n");
   exit(EXIT_FAILURE);
}

int main(int argc, char* argv[])  // main begins
{
   CarbonStartSim(argc, argv);

   for (int i = 0; i < ITERATIONS; i++)
   {
#ifdef DEBUG
      fprintf(stderr, "Starting iteration %d...\n", i);
#endif
      do_cannon(argc, argv);
   }

#ifdef DEBUG
   fprintf(stderr, "Exiting...\n");
#endif

   CarbonStopSim();
}

int do_cannon(int argc, char* argv[])
{
   float **a, **b, **c;

   unsigned int num_threads;
   unsigned int matSize;

   bool found_num_threads = false;
   bool found_mat_size = false;

   for(unsigned int i = 0; i < argc; i++)
   {
       if(strcmp(argv[i],"-m") == 0 && i + 1 < argc)
       {
           matSize = atoi(argv[i+1]);
           found_mat_size = true;
           i += 1;
       }
       else if(strcmp(argv[i],"-t") == 0 && i + 1 < argc)
       {
           num_threads = atoi(argv[i+1]);
           found_num_threads = true;
           i += 1;
       }
   }

   // Read in the command line arguments
   if(!found_num_threads || !found_mat_size)
      printErrorAndExit();

   // Declare threads and related variables
   carbon_thread_t threads[num_threads];
   carbon_barrier_t initialization_barrier;
   carbon_mutex_t debug_lock;

   // Initialize global variables
   CarbonBarrierInit(&initialization_barrier, num_threads+1);
   CarbonMutexInit(&debug_lock);

   // Initialize a
   debug_printf(&debug_lock, "Allocating and Initializing matrix a\n");
   a = (float**)malloc(matSize*sizeof(float*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      a[i] = (float*)malloc(matSize*sizeof(float));
      for (unsigned int j = 0; j < matSize; j++) a[i][j] = 2;
   }

   // Initialize b
   debug_printf(&debug_lock, "Allocating and Initializing matrix b\n");
   b = (float**)malloc(matSize*sizeof(float*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      b[i] = (float*)malloc(matSize*sizeof(float));
      for (unsigned int j = 0; j < matSize; j++) b[i][j] = 3;
   }

   // Initialize c
   debug_printf(&debug_lock, "Allocating and Initializing matrix c\n");
   c = (float**)malloc(matSize*sizeof(float*));
   for (unsigned int i = 0; i < matSize; i++)
   {
      c[i] = (float*)malloc(matSize*sizeof(float));
      for (unsigned int j = 0; j < matSize; j++) c[i][j] = 0;
   }

   // FIXME: we get a compiler warning here because the output of
   // sqrt is being converted to an int.  We really should be doing
   // some sanity checking of num_threads and matSize to be sure
   // these calculations go alright.
   unsigned int blockSize, sqrtNumProcs;
   
   float tmp = sqrt((float) num_threads);
   if (tmp != (float) ((unsigned int) tmp))
      printErrorAndExit();

   sqrtNumProcs = (unsigned int) tmp;
   
   if (matSize % sqrtNumProcs != 0)
      printErrorAndExit();
   
   blockSize = matSize / sqrtNumProcs;

   CAPI_return_t rtnVal;
   rtnVal = CAPI_Initialize(num_threads);

   debug_printf(&debug_lock, "Initializing thread structures\n");

   // Spawn the worker threads
   for (unsigned int i = 0; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(cannon, (void *) i);

   for (unsigned int i = 0; i < num_threads; i++)
   {
      int tid = i;

      // Try to send num_threads to tid. Wait till tid is initialized
      while(1)
      {
         CAPI_return_t ret = CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&num_threads, sizeof(num_threads));
         assert((ret == 0) || (ret == CAPI_ReceiverNotInitialized));
         if (ret == 0)
            break;
         sleep(1);
      } 
      spawner_wait_ack(tid, num_threads);

      assert(
         CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&blockSize, sizeof(blockSize))
         == 0);
      spawner_wait_ack(tid, num_threads);

      assert(
         CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&sqrtNumProcs, sizeof(sqrtNumProcs))
         == 0);
      spawner_wait_ack(tid, num_threads);

      // Send the Initialization Barrier
      assert(
         CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&initialization_barrier, sizeof(initialization_barrier))
         == 0);
      spawner_wait_ack(tid, num_threads);

      // Send the Debug Lock
      assert(
         CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&debug_lock, sizeof(debug_lock))
         == 0);
      spawner_wait_ack(tid, num_threads);

      // Convert 1-D rank to 2-D rank
      int ax, ay, bx, by;
      int x = tid / sqrtNumProcs;
      int y = tid % sqrtNumProcs;

      ax = x * blockSize;
      ay = ((x + y) < sqrtNumProcs)?(x + y):(x + y - sqrtNumProcs);
      ay = ay * blockSize;

      bx = ((x + y) < sqrtNumProcs)?(x + y):(x + y - sqrtNumProcs);
      bx = bx * blockSize;
      by = y * blockSize;
      for (int row = 0; row < blockSize; row++)
      {
         assert(
            CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)&(a[ax + row][ay]), blockSize * sizeof(float))
            == 0);
         spawner_wait_ack(tid, num_threads);
      }

      for (int row = 0; row < blockSize; row++)
      {
         assert(
            CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)&(b[bx + row][by]), blockSize * sizeof(float))
            == 0);
         spawner_wait_ack(tid, num_threads);
      }
   }

   // Send a message to each thread to start execution
   CarbonBarrierWait(&initialization_barrier);

   debug_printf(&debug_lock, "Done sending, waiting for worker threads to complete\n");

   // Wait for all threads to complete
   for (unsigned int i = 0; i < num_threads; i++)
   {
      int tid = i;
      int ax, ay, bx, by;
      int x = tid / sqrtNumProcs;
      int y = tid % sqrtNumProcs;

      ax = x * blockSize;
      ay = ((x + y) < sqrtNumProcs)?(x + y):(x + y - sqrtNumProcs);
      ay = ay * blockSize;

      bx = ((x + y) < sqrtNumProcs)?(x + y):(x + y - sqrtNumProcs);
      bx = bx * blockSize;
      by = y * blockSize;

      spawner_send_go(tid, num_threads);
      float *cRow = (float*)malloc(blockSize*sizeof(float));
      for (unsigned int row = 0; row < blockSize; row++)
      {
         assert(
            CAPI_message_receive_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)num_threads, (char *)cRow, blockSize * sizeof(float))
            == 0);
         spawner_send_go(tid, num_threads);

         for (unsigned int y = 0; y < blockSize; y++) c[ax + row][by + y] = cRow[y];
      }
      free(cRow);

      CarbonJoinThread(threads[i]);
   }

   // Check the result matrix
   for (unsigned int i = 0; i < matSize; i++)
   {
      for (unsigned int j = 0; j < matSize; j++)
      {
         if (c[i][i] != (a[0][0] * b[0][0] * matSize))
            fprintf(stderr, "Error in Matrix Multiplication: c[%u,%u] = %f", i, j, c[i][j]);
      }
   }

   printf("%f\n", c[0][0]);

   for (unsigned int i = 0; i < matSize; i++)
   {
      free(a[i]);
      free(b[i]);
      free(c[i]);
   }

   free(a);
   free(b);
   free(c);
  
   return 0;

} // main ends

void spawner_send_go(int tid, unsigned int num_threads)
{
#ifdef SEQUENTIAL
   bool ack = 1;
   assert(
      CAPI_message_send_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)&ack, sizeof(ack))
      == 0);
#endif
}

void worker_wait_go(int tid, unsigned int num_threads)
{
#ifdef SEQUENTIAL
   bool ack;
   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)&ack, sizeof(ack))
      == 0);
   assert(ack == true);
#endif
}

void spawner_wait_ack(int tid, unsigned int num_threads)
{
#ifdef SEQUENTIAL
   bool ack;
   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)num_threads, (char *)&ack, sizeof(ack))
      == 0);
   assert(ack == true);
#endif
}

void worker_send_ack(int tid, unsigned int num_threads)
{
#ifdef SEQUENTIAL
   bool ack = 1;
   assert(
      CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)num_threads, (char *)&ack, sizeof(ack))
      == 0);
#endif
}

void* cannon(void *threadid)
{
   // Declare local variables
   unsigned int num_threads;
   unsigned int sqrtNumProcs;
   unsigned int blockSize;
   int tid;

   unsigned int upProc, downProc, rightProc, leftProc;
   float **aBlock, **bBlock, **cBlock;
   unsigned int i, j;
   unsigned int ai, aj, bi, bj;
   CAPI_return_t rtnVal;
   carbon_mutex_t debug_lock;
   carbon_barrier_t initialization_barrier;
   
   tid = (int) threadid;
   rtnVal = CAPI_Initialize(tid);

   // Initialize local variables
   assert(
      CAPI_message_receive_w(CAPI_ENDPOINT_ANY, (CAPI_endpoint_t)tid, (char*)&num_threads, sizeof(num_threads))
      == 0);
   worker_send_ack(tid, num_threads);

   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&blockSize, sizeof(blockSize))
      == 0);
   worker_send_ack(tid, num_threads);

   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&sqrtNumProcs, sizeof(sqrtNumProcs))
      == 0);    
   worker_send_ack(tid, num_threads);

   // Receive the initialization barrier
   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&initialization_barrier, sizeof(initialization_barrier))
      == 0);    
   worker_send_ack(tid, num_threads);

   // Receive the debug_lock
   assert(
      CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char*)&debug_lock, sizeof(debug_lock))
      == 0);
   worker_send_ack(tid, num_threads);

   debug_printf(&debug_lock, "Thread(%i): numThreads(%u), blockSize(%u), sqrtNumProcs(%u), initializationBarrier(%i), debugLock(%i)\n",
                             tid, num_threads, blockSize, sqrtNumProcs, initialization_barrier, debug_lock);

   debug_printf(&debug_lock, "Thread %i starting to retrieve initial data\n", tid);

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
   for (int x = 0; x < blockSize; x++)
   {
      aBlock[x] = (float*)malloc(blockSize*sizeof(float));
      assert(
         CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)aBlock[x], blockSize * sizeof(float))
         == 0);
      worker_send_ack(tid, num_threads);
   }

   assert(aBlock[0][0] == 2.0);

   // populate bBlock
   bBlock = (float**)malloc(blockSize*sizeof(float*));
   for (unsigned int x = 0; x < blockSize; x++)
   {
      bBlock[x] = (float*)malloc(blockSize*sizeof(float));
      assert(
         CAPI_message_receive_w((CAPI_endpoint_t)num_threads, (CAPI_endpoint_t)tid, (char *)bBlock[x], blockSize * sizeof(float))
         == 0);
      worker_send_ack(tid, num_threads);
   }

   assert(bBlock[0][0] == 3.0);

   // Allocate cBlock
   cBlock = (float**)malloc(blockSize*sizeof(float*));
   for (int x = 0; x < blockSize; x++) {
      cBlock[x] = (float*)malloc(blockSize*sizeof(float));
      for (unsigned int y = 0; y < blockSize; y++) cBlock[x][y] = 0;
   }

   debug_printf(&debug_lock, "Thread %i finished retrieving initial data, starting computation\n", tid);

   // The main thread will send me a start signal to start
   CarbonBarrierWait(&initialization_barrier);
   
   for (unsigned int iter = 0; iter < sqrtNumProcs; iter++) // for loop begins
   {
      for (unsigned int x = 0; x < blockSize; x++)
         for (unsigned int y = 0; y < blockSize; y++)
            for (unsigned int z = 0; z < blockSize; z++) cBlock[x][y] += aBlock[x][z] * bBlock[z][y];

      if (iter < sqrtNumProcs - 1) // if block begins
      {
         // Send aBlock left

         for (unsigned int x = 0; x < blockSize; x++)
            for (unsigned int y = 0; y < blockSize; y++)
            {
               assert(
                  CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)leftProc, (char*)&aBlock[x][y], sizeof(float))
                  == 0);
            }

         // Send bBlock up
         for (unsigned int x= 0; x < blockSize; x++)
            for (unsigned int y = 0; y < blockSize; y++)
            {
               assert(
                  CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)upProc, (char*)&bBlock[x][y], sizeof(float))
                  == 0);
            }


         // Receive aBlock from right
         for (unsigned int x = 0; x < blockSize; x++)
            for (unsigned int y = 0; y < blockSize; y++)
            {
               assert(
                  CAPI_message_receive_w((CAPI_endpoint_t)rightProc, (CAPI_endpoint_t)tid, (char*)&aBlock[x][y], sizeof(float))
                  == 0);
            }

         // Receive bBlock from below
         for (unsigned int x = 0; x < blockSize; x++)
            for (unsigned int y = 0; y < blockSize; y++)
            {
               assert(
                  CAPI_message_receive_w((CAPI_endpoint_t)downProc, (CAPI_endpoint_t)tid, (char*)&bBlock[x][y], sizeof(float))
                  == 0);
            }

      } // if block ends
   } // for loop ends


   // Update c
   worker_wait_go(tid, num_threads);
   for (unsigned int x = 0; x < blockSize; x++)
   {
      assert(
         CAPI_message_send_w((CAPI_endpoint_t)tid, (CAPI_endpoint_t)num_threads, (char *)(cBlock[x]), blockSize * sizeof(float))
         == 0);
      worker_wait_go(tid, num_threads);
   }

   // Free Scratch Memory
   for (unsigned int i = 0; i < blockSize; i++)
   {
      free(aBlock[i]);
      free(bBlock[i]);
      free(cBlock[i]);
   }

   free(aBlock);
   free(bBlock);
   free(cBlock);
   
   return NULL;
}

