/****************************************************
 * This is a test that will test the pthreads API   *
 *                                                  *
 ****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//pthread_barrier_t barrier; //why does this not work?
int barrier;

pthread_mutex_t mux1;
pthread_mutex_t mux2;
pthread_mutex_t mux3;

// Functions executed by threads
void* thread_func(void * threadid);
void* barrier_func(void * threadid);
void* mutex_func(void * threadid);
void* cond_func(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   // Read in the command line arguments
   const unsigned int numThreads = 3;

   // Declare threads and related variables
   pthread_t threads[numThreads];

   pthread_barrier_init(&barrier, NULL, numThreads);

   for(unsigned int i = 0; i < 2; i++)
   {
      fprintf(stdout, "Spawning thread %d\n", i);

      //spawn test
      for(unsigned int j = 0; j < numThreads; j++)
          pthread_create(&threads[j], NULL, thread_func, (void *) (j + 1));

      for(unsigned int j = 0; j < numThreads; j++)
          pthread_join(threads[j], NULL);

      //barrier test
      for(unsigned int j = 0; j < numThreads; j++)
          pthread_create(&threads[j], NULL, barrier_func, (void *) (j + 1));

      for(unsigned int j = 0; j < numThreads; j++)
          pthread_join(threads[j], NULL);

      //mutex test
      for(unsigned int j = 0; j < numThreads; j++)
          pthread_create(&threads[j], NULL, mutex_func, (void *) (j + 1));

      for(unsigned int j = 0; j < numThreads; j++)
          pthread_join(threads[j], NULL);
      
      //cond test
//      for(unsigned int j = 0; j < numThreads; j++)
//          pthread_create(&threads[j], NULL, cond_func, (void *) (j + 1));

//      for(unsigned int j = 0; j < numThreads; j++)
//          pthread_join(threads[j], NULL);
   }
   
   pthread_barrier_destroy(&barrier);

   fprintf(stdout, "UserApplication: About to exit!\n");

   return 0;
}

void* thread_func(void *threadid)
{
   fprintf(stdout, "Thread Test : Spawned thread #(%d)\n", (int) threadid);
   return NULL;
}

void* barrier_func(void *threadid)
{
   fprintf(stdout, "Barrier Test: Spawned thread #(%d)\n", (int) threadid);

   for (unsigned int i = 0; i < 50; i++)
      pthread_barrier_wait(&barrier);

   return NULL;
}   

void* mutex_func(void *threadid)
{
   fprintf(stdout, "Mutex Test  : Spawned thread #(%d)\n", (int) threadid);
   
   for (unsigned int i = 0; i < 10; i++)
   {
      pthread_mutex_lock(&mux1);
      pthread_mutex_lock(&mux2);
      pthread_mutex_lock(&mux3);
      if (i % 25 == 0)
         fprintf(stderr, "Thread %d got %dth lock...\n", (int) threadid, i);
      pthread_mutex_unlock(&mux3);
      pthread_mutex_unlock(&mux2);
      pthread_mutex_unlock(&mux1);
   }
   
   return NULL;
}

void* cond_func(void *threadid)
{
   fprintf(stdout, "Cond Test : Spawned thread #(%d)\n", (int) threadid);
   fprintf(stdout, "WARNING: Not implemented yet.\n");
}

