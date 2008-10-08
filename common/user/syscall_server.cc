#include "syscall_server.h"


SyscallServer *g_syscall_server = NULL;


void initSyscallServer()
{
   if(!g_syscall_server)
   {
      g_syscall_server = new SyscallServer();
   }
   else
   {
       cout << "Error: tried to double init syscall server" << endl;
   }
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
