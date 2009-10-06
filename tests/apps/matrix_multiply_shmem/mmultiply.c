#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "carbon_user.h"
#include "capi.h"

#define DEBUG 1

float **matA;
float **matB;
float **matC;
SInt32 m, n, p;
SInt32 num_cores;

#ifdef DEBUG
pthread_mutex_t print_lock;
carbon_barrier_t mmultiply_barrier;
#endif

void* threadMain(void*);
void printMatrix(float**, SInt32, SInt32);

int main(int argc, char *argv[])
{
   CarbonStartSim(argc, argv);

   // m n p P
   // p % P == 0
/*    if (argc != 5) */
/*    { */
/*       fprintf(stderr, "[Usage]: ./mmulitply m n p P\n"); */
/*       fprintf(stderr, "m, n, p - Matrix Dimensions\n"); */
/*       fprintf(stderr, "P - Number of Processors\n"); */
/*       fprintf(stderr, "p should be a multiple of P == 0\n"); */
/*       exit(-1); */
/*    } */

   printf("Starting Matrix Multiply\n");

   m = atoi(argv[1]);
   n = atoi(argv[2]);
   p = atoi(argv[3]);
   num_cores = atoi(argv[4]);

   if (p % num_cores != 0)
   {
      fprintf(stderr, "[Usage]: ./mmulitply m n p P\n");
      fprintf(stderr, "p should be a multiple of P == 0\n");
      exit(-1);
   }

   // Initializing matrix elements
   matA = (float**) malloc(m * sizeof(float*));
   for (SInt32 i = 0; i < m; i++)
   {
      matA[i] = (float*) malloc(n * sizeof(float));
      for (SInt32 j = 0; j < n; j++)
      {
         matA[i][j] = i*j;
      }
   }

   matB = (float**) malloc(n * sizeof(float*));
   for (SInt32 i = 0; i < n; i++)
   {
      matB[i] = (float*) malloc(p * sizeof(float));
      for (SInt32 j = 0; j < p; j++)
      {
         matB[i][j] = i+j;
      }
   }

   matC = (float**) malloc(m * sizeof(float*));
   for (SInt32 i = 0; i < m; i++)
   {
      matC[i] = (float*) malloc(p * sizeof(float));
      for (SInt32 j = 0; j < p; j++)
      {
         matC[i][j] = 0.0;
      }
   }


   printf("A = \n");
   printMatrix(matA, m, n);
   printf("B = \n");
   printMatrix(matB, n, p);

   carbon_thread_t threads[num_cores];

#ifdef DEBUG
   pthread_mutex_init(&print_lock, NULL);
   CarbonBarrierInit(&mmultiply_barrier, num_cores);
#endif

   for (SInt32 i = 0; i < num_cores; i++)
   {
      threads[i] = CarbonSpawnThread(threadMain, (void*) i);
      if (threads[i] < 0) {
         fprintf(stderr, "ERROR spawning thread %i!\n", i);
         exit(-1);
      }

#ifdef DEBUG
      pthread_mutex_lock(&print_lock);
      printf("Created Thread: %i\n", i);
      pthread_mutex_unlock(&print_lock);
#endif
   }

   for (SInt32 i = 0; i < num_cores; i++)
   {
      CarbonJoinThread(threads[i]);
   }

   printf("C = \n");
   printMatrix(matC, m, p);
   
   printf("Done. Exiting..\n");

   CarbonStopSim();

   return 0;
}

void* threadMain(void* threadid)
{
   SInt32 tid = (SInt32) threadid;

   SInt32 p_min = tid * (p / num_cores);
   SInt32 p_max = (tid+1) * (p / num_cores);

   for (SInt32 i = 0; i < m; i++)
   {
      for (SInt32 j = p_min; j < p_max; j++)
      {
         matC[i][j] = 0.0;
         for (SInt32 k = 0; k < n; k++)
         {
            matC[i][j] += matA[i][k] * matB[k][j];
         }
      }
   }

#ifdef DEBUG
   CarbonBarrierWait(&mmultiply_barrier);

   if (tid == 0)
   {
      printf("C = \n");
      printMatrix(matC, m, p);
   }

   CarbonBarrierWait(&mmultiply_barrier);
#endif

   return 0;
}

void printMatrix(float** mat, SInt32 num_rows, SInt32 num_cols)
{
   for (SInt32 i = 0; i < num_rows; i++)
   {
      for (SInt32 j = 0; j < num_cols; j++)
      {
         printf("%f\t", mat[i][j]);
      }
      printf("\n");
   }
}
