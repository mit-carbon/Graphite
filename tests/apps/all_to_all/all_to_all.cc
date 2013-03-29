#include <stdio.h>
#include <stdlib.h>
#include "carbon_user.h"
#include <assert.h>

int num_threads;
carbon_barrier_t rank_barrier;

void* thread_func(void* threadid);
void do_bcast(int rank);
void receive_from_all(int rank);

int main(int argc, char **argv)
{
   CarbonStartSim(argc, argv);
   printf("Starting all_to_all!\n");

   if (argc != 2)
   {
      fprintf(stderr, "Usage: ./all_to_all <num_threads>\n");
      CarbonStopSim();
      exit(-1);
   }
   num_threads = atoi(argv[1]);

   // Actually, there are 5 threads including the main thread
   carbon_thread_t threads[num_threads-1];

   CarbonBarrierInit(&rank_barrier, num_threads);

   for(int i = 0; i < num_threads-1; i++)
   {
       printf("Spawning thread: %d\n", i);
       threads[i] = CarbonSpawnThread(thread_func, (void *) i);
   }
  
   thread_func((void*) (num_threads-1));

   for(int i = 0; i < num_threads-1; i++)
       CarbonJoinThread(threads[i]);
   
   printf("Ending all_to_all!\n");
   CarbonStopSim();
   return 0;
}

void* thread_func(void* threadid)
{
	long rank = (long)threadid;

   CAPI_Initialize(rank);
   CarbonBarrierWait(&rank_barrier);

   do_bcast(rank);
   receive_from_all(rank);

   return NULL;
}

void do_bcast(int rank)
{
	int sent_message_payload = rank; 

   CAPI_endpoint_t sender_rank = (CAPI_endpoint_t) rank;
   CAPI_endpoint_t receiver_rank = (CAPI_endpoint_t) CAPI_ENDPOINT_ALL;
		
   printf("[do_bcast] rank %d is broadcasting out message; value = %d\n", rank, sent_message_payload);
   CAPI_message_send_w(sender_rank, receiver_rank, (char *) &sent_message_payload, sizeof(int));
   printf("[do_bcast] rank %d finished broadcasting out message\n", rank);
}

void receive_from_all(int rank)
{
   for (int i = 0; i < num_threads; i++)
   {
      int received_message_payload = -1;
      CAPI_endpoint_t sender_rank = (CAPI_endpoint_t) CAPI_ENDPOINT_ANY;
      CAPI_endpoint_t receiver_rank = (CAPI_endpoint_t) rank;

      printf("[receive_from_all] rank %d is about to receive message\n", rank);
      CAPI_message_receive_w(sender_rank, receiver_rank, (char*) &received_message_payload, sizeof(int));
      printf("[receive_from_all] rank %d received message; value = %d\n", rank, received_message_payload);
   }
}
