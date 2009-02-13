/* Simple message-passing test program for kLab simulator adapted from
 * OpenMPI sample program ring_c.c by Jason Miller.
 */

/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 *
 * Simple ring test program
 */

//#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "capi.h"

//using namespace std;

#define NUM_LOOPS      30
#define MSG_SIZE       10      // number of integers in each message

int main(int argc, char *argv[])
{
   int rank, size, next, prev;
   int i;
   int message[MSG_SIZE];
   int num_iters = NUM_LOOPS;
   time_t t0, t1;
   CAPI_return_t rtnVal;
   int* foo;

   // Read command line arguments
   if (argc != 3)
   {
      //cout << "Invalid command line options. The correct format is:" << endl;
      //cout << "ring_c -m num_of_cores" << endl;
      exit(EXIT_FAILURE);
   }
   else
   {
      if (strcmp(argv[1], "-m\0") == 0)
      {
         size = atoi(argv[2]);
      }
      else
      {
         //cout << "Invalid command line options. The correct format is:" << endl;
         //cout << "ring_c -m num_of_threads" << endl;
         exit(EXIT_FAILURE);
      }
   }

   // Initialize CAPI
   rtnVal = CAPI_Initialize(&rank);
   CAPI_rank(&rank);

   // Calculate the rank of the next process in the ring.  Use the
   //  modulus operator so that the last process "wraps around" to
   //  rank zero.

   next = (rank + 1) % size;
   prev = (rank + size - 1) % size;

   foo = (int*)malloc(10000*sizeof(int));

   // If we are the "master" process (i.e., rank 0), put the number
   //  of times to go around the ring in the first word of the message.

   if (0 == rank)
   {
      // FIXME: temporarily remove ability to set num_iters on command line
      //if (argc == 2) { num_iters = atoi(argv[1]); }
      message[0] = num_iters;
      for (i = 1; i < MSG_SIZE; i++) { message[i] = rand(); }

      printf("Process 0: %d iterations, %d word message, %d processes in ring\n",
             message[0], MSG_SIZE, size);

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
      //MPI_Recv(&message[0], MSG_SIZE, MPI_INT, prev, tag, MPI_COMM_WORLD,
      //  MPI_STATUS_IGNORE);
      CAPI_message_receive_w((CAPI_endpoint_t)prev, (CAPI_endpoint_t)rank,
                             (char*)&message[0], sizeof(int)*MSG_SIZE);
      //printf("Process %d received message from process %d\n", rank, prev);

      if (0 == rank)
      {
         --message[0];
         printf("Process 0 decremented value: %d\n", message[0]);
      }

      //MPI_Send(&message[0], MSG_SIZE, MPI_INT, next, tag, MPI_COMM_WORLD);
      CAPI_message_send_w((CAPI_endpoint_t)rank, (CAPI_endpoint_t)next,
                          (char*)&message[0], sizeof(int)*MSG_SIZE);
      //printf("Process %d sent message to process %d\n", rank, next);
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
      //MPI_Recv(&message[0], MSG_SIZE, MPI_INT, prev, tag, MPI_COMM_WORLD,
      //          MPI_STATUS_IGNORE);
      CAPI_message_receive_w((CAPI_endpoint_t)prev, (CAPI_endpoint_t)rank,
                             (char*)&message[0], sizeof(int)*MSG_SIZE);
      t1 = time(NULL);

      printf("Elapsed wall clock time: %ld sec\n", (long)(t1-t0));
   }

   // All done

   //MPI_Finalize();
   return 0;
}
