/****************************************************
 * This is a test that will use system calls to open
 * a file, read from it, write to it and close it.
 *
 ****************************************************/

#include <stdio.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>

#include "capi.h"
#include <stdio.h>

void* read_and_write(void * threadid);

int main(int argc, char* argv[])  // main begins
{
   const unsigned int numThreads = 2;

   int threads[numThreads];
   pthread_attr_t attr;

   for (unsigned int i = 0; i < numThreads; i++)
      threads[i] = CarbonSpawnThread(read_and_write, (void*)i);

   for (unsigned int i = 0; i < numThreads; i++)
      CarbonJoinThread(threads[i]);

   printf("quitting syscall server!\n");;
}

void* read_and_write(void *threadid)
{
   int tid;
   CAPI_return_t rtnVal;
   char file_name[256];

   rtnVal = CAPI_Initialize((int)threadid);
   CAPI_rank(&tid);

   sprintf(file_name, "./common/tests/file_io/input%d", tid);

   for (int i = 0; i < 100; i++)
   {
      // Open the file
      int fid;
      fid = open(file_name, O_RDWR);
      printf("User %d: File Descriptor: 0x%x\n", tid, (unsigned int)fid);

      // Read the FID
      char m_data[1024] = {'\0'};
      int status = read(fid, (void *) m_data, 1024);
      printf("User %d: Read from fid %d returned %d and %s\n", tid, fid, status, m_data);

      // Close the FID
      status = close(fid);
      printf("User %d: Close from fid %d returned %d\n", tid, fid, status);

      // Open the FID
      fid = open(file_name, O_RDWR | O_TRUNC);
      printf("User %d: File Descriptor: 0x%x\n", tid, (unsigned int)fid);

      // Write the FID
      char m_write_data[12] = "hello world";
      status = write(fid, (void *) m_write_data, 11);
      printf("User %d: Write from fid %d returned %d\n", tid, fid, status);

      // Close the FID
      status = close(fid);
      printf("User %d: Close from fid %d returned %d\n", tid, fid, status);
   }

   pthread_exit(NULL);
}

