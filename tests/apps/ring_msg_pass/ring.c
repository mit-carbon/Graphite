/* Simple message-passing test program for kLab simulator adapted from
 * OpenMPI sample program ring_c.c by Jason Miller. 
 * Later modified to be a thread based version by Charles Gruenwald.
 */

/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 *
 * Simple ring test program
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "carbon_user.h"
#include "capi.h"

#define NUM_LOOPS      30
#define MSG_SIZE       10      // number of integers in each message

// only proc0 really sets this value
// therefore only assume main has access
// to it.
int g_num_threads;

void *ring(void *p);

int main(int argc, char *argv[])
{
   if (argc != 3 || strcmp(argv[1], "-m") != 0)
   {
      printf("Invalid command line options. The correct format is:\n");
      printf("%s -m num_of_threads\n", argv[0]);
      exit(EXIT_FAILURE);
   }

   const unsigned int num_threads = atoi(argv[2]);
   g_num_threads = num_threads;
   if(num_threads < 2)
   {
      printf("Error: need at least 2 threads to perform ring test.");
   }

   carbon_thread_t threads[num_threads];

   CAPI_Initialize(0);

   printf("Spawning worker threads.\n");
   for(unsigned int i = 1; i < num_threads; i++)
       threads[i] = CarbonSpawnThread(ring, (void *) i);

   sleep(3);

   printf("Spawning master thread.\n");
   // Run the master thread.
   ring((void *)0);

   for(unsigned int i = 1; i < num_threads; i++)
       CarbonJoinThread(threads[i]);

   printf("Finished running ring test!.\n");

   return 0;
}

void *ring(void *p)
{
   int next, prev;
   int i;
   int message[MSG_SIZE];
   int num_iters = NUM_LOOPS;
   time_t t0, t1;
   CAPI_return_t rtnVal;
   int* foo;

   int ring_size = g_num_threads;
   int rank = (int)p;

   CAPI_Initialize(rank);

   sleep(5);

   if(rank == 0)
   {
       // Inform the workers of the ring size
       for(unsigned int i = 0; i < g_num_threads; i++)
           CAPI_message_send_w((CAPI_endpoint_t)0, (CAPI_endpoint_t)i,
                   (char*)&g_num_threads, sizeof(g_num_threads));
   }
   else
   {
       CAPI_message_receive_w((CAPI_endpoint_t)0, (CAPI_endpoint_t)i,
               (char*)&g_num_threads, sizeof(g_num_threads));
   }

   ring_size = g_num_threads;
   fprintf(stderr, "Thread: %d/%d started.\n", rank, ring_size);

   // Calculate the rank of the next process in the ring.  Use the
   //  modulus operator so that the last process "wraps around" to
   //  rank zero.

   next = (rank + 1) % ring_size;
   prev = (rank + ring_size - 1) % ring_size;

   foo = (int*)malloc(10000*sizeof(int));

   // If we are the "master" process (i.e., rank 0), put the number
   //  of times to go around the ring in the first word of the message.
   if (0 == rank)
   {
      message[0] = num_iters;
      for (i = 1; i < MSG_SIZE; i++) { message[i] = rand(); }

      printf("Process 0: %d iterations, %d word message, %d processes in ring\n",
             message[0], MSG_SIZE, ring_size);

      t0 = time(NULL);
      CAPI_message_send_w((CAPI_endpoint_t)rank, (CAPI_endpoint_t)next,
                          (char*)&message[0], sizeof(int)*MSG_SIZE);

      printf("Process 0 sent to %d\n", next);
   }

   // Pass the message around the ring.  The exit mechanism works as
   //  follows: the message (a positive integer) is passed around the
   //  ring.  Each time it passes rank 0, it is decremented.  When
   //  each processes receives a message containing a 0 value, it
   //  passes the message on to the next process and then quits.  By
   //  passing the 0 message first, every process gets the 0 message
   //  and can quit normally.

   while (1)
   {
      CAPI_message_receive_w((CAPI_endpoint_t)prev, (CAPI_endpoint_t)rank,
                             (char*)&message[0], sizeof(int)*MSG_SIZE);

      if (0 == rank)
      {
         --message[0];
         printf("Process 0 decremented value: %d\n", message[0]);
      }

      CAPI_message_send_w((CAPI_endpoint_t)rank, (CAPI_endpoint_t)next,
                          (char*)&message[0], sizeof(int)*MSG_SIZE);
      if (0 == message[0])
      {
         printf("Process %d exiting\n", rank);
         break;
      }
   }

   // The last process does one extra send to process 0, which needs
   //  to be received before the program can exit
   if (0 == rank)
   {
      CAPI_message_receive_w((CAPI_endpoint_t)prev, (CAPI_endpoint_t)rank,
                             (char*)&message[0], sizeof(int)*MSG_SIZE);
      t1 = time(NULL);

      printf("Elapsed wall clock time: %ld sec\n", (long)(t1-t0));
   }

   // All done
   return 0;
}

