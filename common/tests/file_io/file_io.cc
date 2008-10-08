/****************************************************
 * This is a test that will use system calls to open 
 * a file, read from it, write to it and close it.
 *
 ****************************************************/

#include <iostream>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include "capi.h"
#include "syscall_server.h"

using namespace std;

bool finished;

#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* read_and_write(void * threadid);
void* server_thread(void *dummy);

int main(int argc, char* argv[]){ // main begins
	
   // Read in the command line arguments
   unsigned int numThreads = 1;

   // Declare threads and related variables
   pthread_t threads[numThreads];
   pthread_attr_t attr;
	
#ifdef DEBUG
   cout << "This is the function main()" << endl;
#endif

   // Initialize threads and related variables
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

#ifdef DEBUG
   cout << "Spawning threads" << endl;
#endif
   // FIXME: for now, this is how we give the syscall server a place to run
   finished = false;
   pthread_t server;
   pthread_create(&server, &attr, server_thread, (void *) 0);

   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_create(&threads[i], &attr, read_and_write, (void *) i);

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_join(threads[i], NULL);

   // FIXME: for now, this is how we terminate the syscall server thread
   finished = true;
   pthread_join(server, NULL);

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends

void* server_thread(void *dummy)
{
   initSyscallServer();

   while( !finished )
   {
      runSyscallServer();
   }   
   pthread_exit(NULL);
}

void* read_and_write(void *threadid)
{

   // Declare local variables
   int tid;
   CAPI_return_t rtnVal;

   rtnVal = CAPI_Initialize(&tid);

   // Initialize local variables
   CAPI_rank(&tid);

   // Do the work
   int fid;
   fid = open("./input", O_RDONLY);

   printf("File Descriptor: 0x%x\n", (unsigned int)fid);

   // Actually read and close the FID
   // int data_size = 1024 * sizeof(char);
   // char *the_data = (char *)malloc(data_size);
   // int read = fread(the_data,data_size, sizeof(char), fid);

   // fclose(fid);

   // printf("Read: %s\n", the_data);

   pthread_exit(NULL);
}

