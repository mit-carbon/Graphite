#include <pthread.h>
#include "syscall_server.h"

// Thread function
void* server_thread(void *dummy);

// Globals required for the syscall server
SyscallServer *g_syscall_server = NULL;
bool finished;
pthread_t server;

void initSyscallServer()
{
   // Make sure this hasn't been called yet
   if(g_syscall_server)
   {
       cout << "Error: tried to double init syscall server" << endl;
       return;
   }

   // Create the syscall server object
   g_syscall_server = new SyscallServer();

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
   }   
   pthread_exit(NULL);
}

void runSyscallServer() 
{
   if(g_syscall_server)
   {
      g_syscall_server->run();
   }
   else
   {
      cout << "Error: tried to run syscall server without initializing it." << endl;
   }
}

void finiSyscallServer()
{
   if(g_syscall_server)
   {
      // FIXME: for now, this is how we terminate the syscall server thread
      finished = true;
      pthread_join(server, NULL);
      delete g_syscall_server;
   }
}


void SyscallServer::run()
{
  // using PT_get_from_server_buff and PT_put_to_server_reply_buff, pull syscall
  // messages from queue in PT, process them, then marshall results back
  // via another queue in PT

  cout << "made it to syscall handler" << endl;
}
