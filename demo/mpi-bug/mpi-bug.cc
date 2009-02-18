#include <stdio.h>
#include <mpi.h>
#include <assert.h>
#include <string.h>
#include <sched.h>

int rank, num_processes;
const char *msg = "Hello.";

void init()
{
   fprintf(stderr, "Doing MPI stuff.\n");

   int provided;

   provided = 0;
   rank = 0;
   num_processes = 0;

   // Initialize
   MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);
   assert(provided >= MPI_THREAD_MULTIPLE);

   MPI_Comm_rank(MPI_COMM_WORLD, &rank);

   MPI_Comm_size(MPI_COMM_WORLD, &num_processes);
   fprintf(stderr, "Initialized process %d out of %d.\n", rank, num_processes);
}

void send()
{
   // Send message
   int size = strlen(msg) + 1;

   MPI_Send((void*)msg, size, MPI_BYTE, num_processes - 1, 0, MPI_COMM_WORLD);
   fprintf(stderr, "Sent message.\n");
}

void recv()
{
   // Receive message
   MPI_Status status;
   char buffer[64];
   int size = strlen(msg)+1;

   MPI_Probe(MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
   fprintf(stderr, "Past probe.\n");

   MPI_Recv(buffer, size, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);
   fprintf(stderr, "Past recv; got: \"%s\".\n", buffer);
}


void finish(int code, void * v)
{
   MPI_Finalize();
   fprintf(stderr, "Finalized.\n");
}

int thread_func(void *)
{
   fprintf(stderr, "In spawned thread.\n");
   recv();
   return 0;
}

void do_mpi_stuff()
{
   init();

   switch (num_processes)
   {
   case 1:
   {
      fprintf(stderr, "Spawning thread.\n");

      // spawn a thread
      const int STACKSIZE = 1000000;
      char *stack = new char[STACKSIZE];

      clone(thread_func,
            stack+STACKSIZE-1,
            CLONE_VM|CLONE_FS|CLONE_FILES|CLONE_SIGHAND,
            NULL);

      send();

      return;
   }

   case 2:
   {
      if (rank == 0)
         send();
      else
         recv();

      return;
   }

   default:
   {
      assert(false);
   }
   };
}

#ifdef USE_PIN

#include "pin.H"

int main(int argc, char *argv[])
{
   fprintf(stderr, " *** Running with PIN. ***\n");

   PIN_Init(argc, argv);

   PIN_AddFiniFunction(finish, 0);

   PIN_StartProgram();

   return 0;
}

#else

int main()
{
   fprintf(stderr, " *** Running without PIN. ***\n");

   do_mpi_stuff();

   finish(0, NULL);

   return 0;
}

#endif // PIN
