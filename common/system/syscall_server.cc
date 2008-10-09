#include "syscall_server.h"

// Thread function
void* server_thread(void *dummy);


SyscallServer::SyscallServer()
{
//   pt_endpt.ptInitMCP();
}

void SyscallServer::run()
{
  // using PT_get_from_server_buff and PT_put_to_server_reply_buff, pull syscall
  // messages from queue in PT, process them, then marshall results back
  // via another queue in PT

   cout << "made it to syscall handler" << endl;
}
