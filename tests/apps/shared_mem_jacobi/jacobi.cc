/**
   i allow core#0 to print out progress
 * This code is being re-purposed for testing the manycore simulator's shared memory
 * (originall written in c)
 */

/**
 * CHRISTOPHER CELIO, FEB 16, 2008
 * 6.846
 *
 * FOLLOWING CODE IS FOR 16 CORE,
 * 1D JACOBIAN RELAXATION. COPIED
 * FROM HW1, AND MODIFIED TO HANDLE
 * 16 CORES.
 *
 * Uses Blocking Messages, staggered
 * send and receive for communications.
 *
 * OUTPUT-(ITERS 12): (output in rows of 12)
 * 25376 19040 12704 8744 4784 3024 1264 736 208 112 16 8
 * 0 0 0 0 0 0 0 0 0 0 0 0
 * 0 0 0 0 0 0 0 0 0 0 0 0
 * 0 0 0 0 0 0 0 0 0 0 0 0
 * 0 0 0 0 0 0 0 0 0 0 0 0
 * 0 0 0 0
 */

#include <stdio.h>
#include "capi.h"

#include "fixed_types.h"

#include <pthread.h> //not sure this is needed

//set by command-line argument
unsigned int coreCount;

#define SIZE (64*8)
//arraysize and the allotment of boundary cells
//is taken care of in the spawning logic

#define ITERS (64*8)
#define SWAP(a,b,t) (((t) = (a)), ((a) = (b)), ((b) = (t)))

//all of the threads run this function
void* thread_main(void *threadid);

void print_array(int *array, int array_size)
{
   int i;
   for (i = 0; i < array_size; ++i)
   {
      if (i % 12 == 0) printf("\n");
      printf("%d ", array[i]);
   }

   printf("\n");
}

void update_array(int *oldarr, int *newarr, int array_size,
                  int my_rank, int left_info, int right_info)
{
   int from, to, i;
   from = (my_rank==0) ? 1 : 0;
   to = ((UInt32) my_rank == (coreCount-1)) ? array_size-2 : array_size -1;

   int left, right;

   for (i = from; i <= to; i++)
   {
      left = (i==0)    ? left_info : oldarr[i-1];
      right = (i==array_size-1) ? right_info : oldarr[i+1];

      newarr[i] = (left + right) /2;
   }

}

void BARRIER(int tid)
{
   int payload; //unused

   if (coreCount > 1)
   {
      if (tid==0)
      {
         printf("*********\ntid0 is beginning to receive messages for barrier.\n");
         //gather all receiver messages, then send a continue to all cores
         for (UInt32 i = 1; i < coreCount; i++)
         {
            printf("core0 is waiting for[%d]\n", i);
            CAPI_message_receive_w(i, 0, (char *) &payload, sizeof(int));
         }
         for (UInt32 i = 1; i < coreCount; i++)
         {
            printf("core0 is waiting for[%d]\n", i);
            CAPI_message_send_w(0, i, (char *) &payload, sizeof(int));
         }
      }
      else
      {
         CAPI_message_send_w(tid, 0, (char *) &payload, sizeof(int));
         CAPI_message_receive_w(0, tid, (char *) &payload, sizeof(int));
      }
   }
   if (tid==0) printf(" YAH! FINISHED WITH BARRIER: \n");
}

int send_info(int my_rank, int receiver_rank, int *info_ptr)
{
   /*
   int MSG_ID = 0;

   if(ilib_msg_send(group, receiver_rank, MSG_ID,
    info_ptr, sizeof(int))
    != ILIB_SUCCESS)
     ilib_die("Failed to send message to proc %d", receiver_rank);
   */

   CAPI_message_send_w(my_rank, receiver_rank, (char*) info_ptr, sizeof(int));

   return 0;
}

int receive_info(int my_rank, int sender_rank)
{
   /*
   int MSG_ID = 0;
   int info;
   ilibStatus status;

   if( ilib_msg_receive(group, sender_rank, MSG_ID,
    &info, sizeof(int), &status)
    != ILIB_SUCCESS)
     ilib_die("Failed to receive message from proc %d", sender_rank);
   */

   int info;

   CAPI_message_receive_w(sender_rank, my_rank, (char*) &info, sizeof(int));

   return info;
}

/* all procs(>0) send their private arrays
 * to proc0 at the end of the game
 */
int send_array(int my_rank, int *arr, int array_size)
{
   /*
   int MSG_ID = 0;

   if(ilib_msg_send(group, 0, MSG_ID, arr, sizeof(int)*array_size)
    != ILIB_SUCESS)
    ilib_die("Failed to send array to proc#0.\n");
   */

   CAPI_message_send_w(my_rank, 0, (char*) arr, sizeof(int)*array_size);
   return 0;
}

/*
 * proc#0 receives all other proc's private arrays
 * and consolidates them into a single, "global" array
 */
int consolidate_arrays(int *global_array, int *proc0_arr)
{

// int MSG_ID = 0;
// ilibStatus status;
   int my_rank = 0;
   int temp_arr[SIZE/coreCount+ 1];
   int temp_size;
   int offset;

   //place proc#0's array info into the global array
   //ignore boundary condition at i=0 in proc0_arr
   for (unsigned int i=0; i < (SIZE/coreCount) + 1; i++)
   {
      global_array[i] = proc0_arr[i+1];
   }

   //for every core, get its personal array, add it to global_array
   for (unsigned int i=1; i < coreCount; i++)
   {

      temp_size = (i== coreCount -1) ? (SIZE/coreCount) + 1
                  : (SIZE/coreCount);

      /*
      if( ilib_msg_receive(group, i, MSG_ID,
       temp_arr, sizeof(int)*temp_size, &status)
       != ILIB_SUCCESS)
        ilib_die("Failed to receive message from proc %d", i);
      */

      CAPI_message_receive_w(i, my_rank, (char*) temp_arr, sizeof(int)*temp_size);

      offset = i*(SIZE/coreCount);

      for (unsigned int j = 0; j < (SIZE/coreCount); j++)
      {
         global_array[j  + offset] = temp_arr[j];
      }
   }

   return 0;
}

void* thread_main(void *threadid)
{
   int tid;
   CAPI_Initialize_FreeRank(&tid);

   int t;
   int *oldarr, *newarr, *tmp;
// int start, stop;
   //this is the two integers from the left and right boundaries.
   int neighbor_left_info = 0, neighbor_right_info = 0;
   int my_left = 0,  my_right = 0;

   unsigned int my_rank = tid;
   unsigned int myArraySize = 0;

   //each tile allocates its own personal, tiny arrays.
   if (coreCount == 1)
   {
      //if only one core, it must have *two* boundary cells
      myArraySize = SIZE + 2;
   }
   else
   {
      if (my_rank==0 || my_rank==coreCount-1)
      {
         //plus one accounts for the boundary cell needed by boundary procs
         myArraySize = (SIZE/coreCount) + 1;
      }
      else
      {
         myArraySize = (SIZE/coreCount);
      }
   }

   //TODO BUG!!!! malloc will lock everyone
   oldarr = (int *) malloc(myArraySize * sizeof(int));
   newarr = (int *) malloc(myArraySize * sizeof(int));

   //initialize every element to zero
   for (UInt32 i = 0; i < myArraySize-1; i++)
//  oldarr[i] = newarr[i] = 0.0;
      oldarr[i] = newarr[i] = 0;

   //initialize the very first element in the
   //entire array
   if (my_rank==0)
   {
//  oldarr[0] = newarr[0] = 32768.0;
      oldarr[0] = newarr[0] = 32768;
   }

// ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
   BARRIER(tid);
   //TODO get a cycle counter?
// start = get_cycle_count();

   for (t = 0; t < ITERS; ++t)
   {

      /**************************
       **************************/
      if (tid==0) printf("--Iteration[%d of %d]--\n", t, ITERS);

      /**************************
       * UPDATE ARRAY
       * send the two neighboring integers in as well.
       * the assumption here is that neighboring tiles
       * are initialized to 0 for the first iteration.
       **************************/
      update_array(oldarr, newarr,myArraySize, my_rank
                   , neighbor_left_info, neighbor_right_info);

      /**************************
       * PERFORM COMMUNICATIONS
       * even procs send first, receive second
       * odd procs receive first, send second.
       **************************/
      //communicate boundaries.
      int left_index = 0;
      int right_index = myArraySize-1;
      my_left = newarr[left_index];
      my_right = newarr[right_index];

      if (my_rank % 2 == 0)
      {
         //send my boundary information to my neighbor
         if (my_rank > 0)
            send_info(my_rank, my_rank-1, &my_left);
         if (my_rank < coreCount-1)
            send_info(my_rank, my_rank+1, &my_right);

         //now let's receive the information from my neighbor
         neighbor_left_info = (my_rank > 0) ?
                              receive_info(my_rank, my_rank-1)
                              : -1337;
         neighbor_right_info = (my_rank < coreCount-1) ?
                               receive_info(my_rank, my_rank+1)
                               : -1338;

      }
      else
      {
         //i'm an odd proc
         //let's receive the information from my neighbor first
         neighbor_left_info = (my_rank > 0) ?
                              receive_info(my_rank, my_rank-1)
                              : -1337;
         neighbor_right_info = (my_rank < coreCount-1) ?
                               receive_info(my_rank, my_rank+1)
                               : -1338;

         if (my_rank > 0)
            send_info(my_rank, my_rank-1, &my_left);
         if (my_rank < coreCount-1)
            send_info(my_rank, my_rank+1, &my_right);
      }

      //swap to begin next round
      SWAP(oldarr, newarr, tmp);

   }


   /*************************************
    * THE END (SEND INFORMATION TO PROC#0
    *************************************/
// ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
// TODO we need a real BARRIER
   BARRIER(tid);

   if (my_rank == 0)
   {
//  stop = get_cycle_count();
//  printf("\nProgram took %d cycles\n", stop-start);
//  printf("Program took %f (s)\n", ((float) (stop-start)) / PROC_CLK_FREQ);

      int global_array[SIZE];
      consolidate_arrays(global_array, oldarr);
      printf("\n****GLOBAL ARRAY****\n  (ITERATIONS: %d)\n  (SIZE: %d)\n  (CORE_COUNT: %d)\n", ITERS, SIZE, coreCount);
      print_array(global_array, SIZE);
      printf("\n");
   }
   else
   {
      send_array(my_rank, oldarr, myArraySize);
   }

   pthread_exit(NULL);

   return 0;
}

//create the threads, have them run thread_main
int main(int argc, char* argv[])
{
   CarbonStartSim();

   // Read in the command line arguments

   if (argc != 3)
   {
      cout << "Invalid command line options. The correct format is:" << endl;
      cout << "jacobi -n num_of_threads" << endl;
      exit(EXIT_FAILURE);
   }
   else if ((strcmp(argv[1], "-n\0") == 0))
   {
      coreCount = atoi(argv[2]);
   }
   else
   {
      cout << "Invalid command line options. The correct format is:" << endl;
      cout << "jacobi -n num_of_threads" << endl;
      exit(EXIT_FAILURE);
   }

   // Declare threads and related variables
   pthread_t threads[coreCount];
   pthread_attr_t attr;

   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   for (unsigned int i=0; i < coreCount; i++)
   {
      pthread_create(&threads[i], &attr, thread_main, (void *) i);
   }

   for (unsigned int i=0; i < coreCount; i++)
   {
      pthread_join(threads[i], NULL);
   }

   CarbonStopSim();
   return 0;

}
