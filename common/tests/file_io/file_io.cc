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
#include "syscall_api.h"

using namespace std;


#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* read_and_write(void * threadid);

int main(int argc, char* argv[]){ // main begins

   initSyscallServer();
	
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

   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_create(&threads[i], &attr, read_and_write, (void *) i);

   // Wait for all threads to complete
   for(unsigned int i = 0; i < numThreads; i++) 
      pthread_join(threads[i], NULL);

   cout << "quitting syscall server!" << endl;
   quitSyscallServer();

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends


void* read_and_write(void *threadid)
{

   // Declare local variables
   int tid;
   CAPI_return_t rtnVal;

   rtnVal = CAPI_Initialize(&tid);

   // Initialize local variables
   CAPI_rank(&tid);

   // Open the file
   int fid;
   fid = open("./common/tests/file_io/input2", O_RDWR);

   printf("File Descriptor: 0x%x\n", (unsigned int)fid);

   // Read the FID
   char the_data[1024] = {'\0'};
   int status = read(fid, (void *) the_data, 1024);
   printf("Read from fid %d returned %d and %s\n", fid, status, the_data);

   // Write the FID
   char the_write_data[14] = "goodbye world";
   status = write(fid, (void *) the_write_data, 13); 
   printf("Write from fid %d returned %d\n", fid, status);

   // Close the FID
   // fclose(fid);

   // printf("Read: %s\n", the_data);

   pthread_exit(NULL);
}

