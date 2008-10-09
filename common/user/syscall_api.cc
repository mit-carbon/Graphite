#include "syscall_api.h"
#include <pthread.h>
#include <iostream>
#include <unistd.h>

using namespace std;

// Globals required for the syscall server
volatile static bool finished = false;
static pthread_t server;

void initSyscallServer()
{
   // Create the syscall server thread
   pthread_attr_t attr;
   pthread_attr_init(&attr);
   pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

   // FIXME: for now, this is how we give the syscall server a place to run
   finished = false;
   pthread_create(&server, &attr, server_thread, (void *) 0);
}

void* server_thread(void *dummy)
{

   while( !finished )
   {
      runSyscallServer();
      usleep(1);
   }   
   pthread_exit(NULL);
}

void runSyscallServer()
{
   cout << "Made it to the dummy function." << endl;
}

void quitSyscallServer()
{
   finished = true;
   pthread_join(server, NULL);
}

