#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "carbon_user.h"

// Functions executed by threads
void* thread_func(void * threadid);

int main(int argc, char* argv[])
{
   CarbonStartSim(argc, argv);

   fprintf(stderr, "Yeah main\n");

   int tid1 = CarbonSpawnThread(thread_func, (void*)5);
   int tid2 = CarbonSpawnThread(thread_func, (void*)10);

   sleep(2);

   CarbonJoinThread(tid1);
   CarbonJoinThread(tid2);

   fprintf(stderr, "After joining...\n");
   CarbonStopSim();
   return 0;
}

void* thread_func(void *threadid)
{
   fprintf(stderr, "Yeah thread\n");
   sleep((int)threadid);
   return 0;
}
