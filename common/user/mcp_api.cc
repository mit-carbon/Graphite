#include "mcp_api.h"
#include <pthread.h>
#include <iostream>
#include <unistd.h>

using namespace std;

// Globals required for the syscall server
volatile static bool finished = false;
static pthread_t mcp_thread;


void initMCP()
{
   // Create the syscall server thread
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   // FIXME: for now, this is how we give the syscall server a place to run
   finished = false;
   pthread_create(&mcp_thread, &attr, mcp_thread_func, (void *) 0);
}

void* mcp_thread_func(void *dummy)
{

   while( !finished )
   {
      runMCP();
      usleep(1);
   }   
   pthread_exit(NULL);
}

void runMCP()
{
   cout << "Made it to the dummy function." << endl;
}

void quitMCP()
{
   finished = true;
   pthread_join(mcp_thread, NULL);
}

