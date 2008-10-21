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
   const unsigned int numThreads = 2;

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
  // Declare local variables
  int tid;
  CAPI_return_t rtnVal;
  char file_name[256];

  rtnVal = CAPI_Initialize(&tid);

  // Initialize local variables
  CAPI_rank(&tid);

  sprintf(file_name, "./common/tests/file_io/input%d", tid);
  
  for (int i = 0; i < 100; i++)
    {
      // Open the file
      int fid;
      fid = open(file_name, O_RDWR);
      printf("User %d: File Descriptor: 0x%x\n", tid, (unsigned int)fid);

      // Read the FID
      char the_data[1024] = {'\0'};
      int status = read(fid, (void *) the_data, 1024);
      printf("User %d: Read from fid %d returned %d and %s\n", tid, fid, status, the_data);

      // Close the FID
      status = close(fid);
      printf("User %d: Close from fid %d returned %d\n", tid, fid, status);

      // Open the FID
      fid = open(file_name, O_RDWR | O_TRUNC);
      printf("User %d: File Descriptor: 0x%x\n", tid, (unsigned int)fid);

      // Write the FID
      char the_write_data[12] = "hello world";
      status = write(fid, (void *) the_write_data, 11); 
      printf("User %d: Write from fid %d returned %d\n", tid, fid, status);

      // Close the FID
      status = close(fid);
      printf("User %d: Close from fid %d returned %d\n", tid, fid, status);
    }

  pthread_exit(NULL);
}

