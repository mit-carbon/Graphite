#include <pthread.h>

/* By changing the values of these constants, you can change the time
 * spent in the main thread before spawning the worker thread and the
 * time spent before joining on the worker thread. This can be used to
 * check that clocks are correctly synchronized on spawn and join.
 *
 * Do the following: Set both to 0. Then change MAIN to 100k. Both
 * clocks should jump, but the instruction count for main thread ONLY
 * should increase. Then set THREAD to 100k. Both clocks should jump
 * again, but now instruction counts should both be larger.
 */

#define MAIN_ITERATIONS  0 
#define THREAD_ITERATIONS  100000

void* f(void*)
{
   int j = 0;
   for (int i = 0; i < THREAD_ITERATIONS; i++)
   {
      j += i;
   }
   return (void*)j;
}

int main()
{
   int j = 0;
   for (int i = 0; i < MAIN_ITERATIONS; i++)
   {
      j += i;
   }

   pthread_t t;
   pthread_create(&t, NULL, f, NULL);
   pthread_join(t, NULL);

   return j;
}
