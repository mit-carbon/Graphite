#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

const int NUM_THREADS = 3;

void* threadFunc(void* arg);

int main()
{
   printf("Starting: pthread_copy test\n");

   pthread_t threads[NUM_THREADS];
   
   // Create threads
   for (int i = 0; i < NUM_THREADS; i++)
   {
      pthread_t thread;
      int ret = pthread_create(&thread, NULL, threadFunc, (void*) i);
      if (ret != 0)
      {
         fprintf(stderr, "Ending: pthread_copy test [FAILED] pthread_create\n");
         exit(EXIT_FAILURE);
      }

      threads[i] = thread;
   }

   // Join threads
   for (int i = 0; i < NUM_THREADS; i++)
   {
      pthread_join(threads[i], NULL);
   }

   printf("Ending: pthread_copy test [PASSED]\n");

   return 0;
}

void* threadFunc(void* arg)
{
   long int tid = (long int) arg;
   printf("Spawned thread(%li)\n", tid);
   return NULL;
}
