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
#include "mcp_api.h"

using namespace std;


#ifdef DEBUG
   pthread_mutex_t lock;
#endif

// Functions executed by threads
void* read_and_write(void * threadid);

int main(int argc, char* argv[]){ // main begins

   initMCP();
	
   // Read in the command line arguments
   const unsigned int numThreads = 1;

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
   quitMCP();

#ifdef DEBUG
   cout << "This is the function main ending" << endl;
#endif
   pthread_exit(NULL);

} // main ends


void* read_and_write(void *threadid)
{

   for (int i = 0; i < 100; i++)
   {
      // Declare local variables
      int tid;
      CAPI_return_t rtnVal;

      rtnVal = CAPI_Initialize(&tid);

      // Initialize local variables
      CAPI_rank(&tid);

      // Open the file
      int fid;
      fid = open("./common/tests/file_io/input", O_RDWR);
      printf("User: File Descriptor: 0x%x\n", (unsigned int)fid);

      // Read the FID
      char the_data[1024] = {'\0'};
      int status = read(fid, (void *) the_data, 1024);
      printf("User: Read from fid %d returned %d and %s\n", fid, status, the_data);

      // Close the FID
      status = close(fid);
      printf("User: Close from fid %d returned %d\n", fid, status);

      // Open the FID
      fid = open("./common/tests/file_io/input", O_RDWR | O_TRUNC);
      printf("User: File Descriptor: 0x%x\n", (unsigned int)fid);

      // Write the FID
      char the_write_data[12] = "hello world";
      status = write(fid, (void *) the_write_data, 11); 
      printf("User: Write from fid %d returned %d\n", fid, status);

      // Close the FID
      status = close(fid);
      printf("User: Close from fid %d returned %d\n", fid, status);
   }

   pthread_exit(NULL);
}

