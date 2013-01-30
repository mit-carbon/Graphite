/****************************************************
 * This is a test that will test the pthreads API   *
 *                                                  *
 ****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

const int numThreads = 3;

pthread_barrier_t barrier;

pthread_mutex_t mux1;
pthread_mutex_t mux2;
pthread_mutex_t mux3;

pthread_cond_t cond;
pthread_mutex_t mux_cond;
int counter;
bool cycle;

// Functions executed by threads
void* thread_func(void * threadid);
void* barrier_func(void * threadid);
void* mutex_func(void * threadid);
void* cond_broadcast_func(void * threadid);
void* cond_signal_func(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   // Declare threads and related variables
   pthread_t threads[numThreads];

   // Initialize for barrier test
   pthread_barrier_init(&barrier, NULL, numThreads);
   
   // Initialize for mutex test
   pthread_mutex_init(&mux1, NULL);
   pthread_mutex_init(&mux2, NULL);
   pthread_mutex_init(&mux3, NULL);

   // Initialize for condition variable test
   pthread_cond_init(&cond, NULL);
   pthread_mutex_init(&mux_cond, NULL);
   counter = 0;
   cycle = false;

   fprintf(stdout, "Starting pthread tests.\n");

   for(int i = 0; i < 2; i++)
   {
      //spawn test
      for(int j = 0; j < numThreads; j++)
         pthread_create(&threads[j], NULL, thread_func, (void *) (j + 1));

      for(int j = 0; j < numThreads; j++)
         pthread_join(threads[j], NULL);

      //barrier test
      for(int j = 0; j < numThreads; j++)
         pthread_create(&threads[j], NULL, barrier_func, (void *) (j + 1));

      for(int j = 0; j < numThreads; j++)
         pthread_join(threads[j], NULL);

      //mutex test
      for(int j = 0; j < numThreads; j++)
         pthread_create(&threads[j], NULL, mutex_func, (void *) (j + 1));

      for(int j = 0; j < numThreads; j++)
         pthread_join(threads[j], NULL);
      
      //cond broadcast test
      for(int j = 0; j < numThreads; j++)
         pthread_create(&threads[j], NULL, cond_broadcast_func, (void *) (j + 1));

     for(int j = 0; j < numThreads; j++)
         pthread_join(threads[j], NULL);
      
      //cond signal test
      for(int j = 0; j < 2; j++)
         pthread_create(&threads[j], NULL, cond_signal_func, (void *) (j + 1));

     for(int j = 0; j < 2; j++)
         pthread_join(threads[j], NULL);
   }
 
   fprintf(stdout, "Finished pthread tests.\n");

   // De-initialize for barrier test 
   pthread_barrier_destroy(&barrier);

   // De-initialize for mutex test
   pthread_mutex_destroy(&mux1);
   pthread_mutex_destroy(&mux2);
   pthread_mutex_destroy(&mux3);

   // De-initialize for cond test
   pthread_cond_destroy(&cond);
   pthread_mutex_destroy(&mux_cond);

   fprintf(stdout, "UserApplication: About to exit!\n");

   return 0;
}

void* thread_func(void *threadid)
{
   fprintf(stdout, "Thread Test : Spawned thread #(%li)\n", (long) threadid);
   return NULL;
}

void* barrier_func(void *threadid)
{
   fprintf(stdout, "Barrier Test: Spawned thread #(%li)\n", (long) threadid);

   for (unsigned int i = 0; i < 50; i++)
      pthread_barrier_wait(&barrier);

   return NULL;
}   

void* mutex_func(void *threadid)
{
   fprintf(stdout, "Mutex Test  : Spawned thread #(%li)\n", (long) threadid);
   
   for (unsigned int i = 0; i < 50; i++)
   {
      pthread_mutex_lock(&mux1);
      pthread_mutex_lock(&mux2);
      pthread_mutex_lock(&mux3);
      
      pthread_mutex_unlock(&mux3);
      pthread_mutex_unlock(&mux2);
      pthread_mutex_unlock(&mux1);
   }
   
   return NULL;
}

void* cond_broadcast_func(void *threadid)
{
   fprintf(stdout, "Condition Variable Broadcast Test : Spawned thread #(%li)\n", (long) threadid);

   for (unsigned int i = 0; i < 50; i++)
   {
      pthread_mutex_lock(&mux_cond);
      bool cycle_ = cycle;
      
      if (++counter != numThreads)
      {
         while (cycle_ == cycle)
         {
            int error = pthread_cond_wait(&cond, &mux_cond);
            if (error != 0)
               break;
         }
      }
      else
      {
         cycle = !cycle;
         counter = 0;
         pthread_cond_broadcast(&cond);
      }
      pthread_mutex_unlock(&mux_cond);
   }

   return NULL;
}

void* cond_signal_func(void *threadid)
{
   fprintf(stdout, "Condition Variable Signal Test : Spawned thread #(%li)\n", (long) threadid);

   for (unsigned int i = 0; i < 50; i++)
   {
      pthread_mutex_lock(&mux_cond);
      bool cycle_ = cycle;
      
      if (++counter != 2)
      {
         while (cycle_ == cycle)
         {
            int error = pthread_cond_wait(&cond, &mux_cond);
            if (error != 0)
               break;
         }
      }
      else
      {
         cycle = !cycle;
         counter = 0;
         pthread_cond_signal(&cond);
      }
      pthread_mutex_unlock(&mux_cond);
   }

   return NULL;
}
