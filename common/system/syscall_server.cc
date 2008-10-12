#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "syscall_server.h"
#include "sys/syscall.h"



// Thread function
void* server_thread(void *dummy);


SyscallServer::SyscallServer()
{
   pt_endpt.ptInitMCP();
}

void SyscallServer::run()
{
   // using PT_get_from_server_buff and PT_put_to_server_reply_buff, pull syscall
   // messages from queue in PT, process them, then marshall results back
   // via another queue in PT

   UInt32 len;
   UInt8* buf = pt_endpt.ptMCPRecv(&len); 
  
   UInt32 type = ((UInt32*)buf)[0];
   UInt32 comm_id = ((UInt32*)buf)[1];

   switch(type)
   {
      case 0:
         handleSyscall(comm_id, &buf[2 * sizeof(UInt32)]);
         break;
   }
  
   delete [] buf;

}

void SyscallServer::handleSyscall(UInt32 comm_id, UInt8* buf)
{
   UInt8 syscall_number = (int)buf[0];

   switch(syscall_number)
   {
      case SYS_open:
      {
         marshallOpenCall(comm_id, (UInt8*)&buf[1]);
         break;
      }
      default:
      {
         cout << "Unhandled syscall number: " << (int)syscall_number << " from: " << comm_id << endl;
      }
   }
}

void SyscallServer::marshallOpenCall(UInt32 comm_id, UInt8* buf)
{
   cout << "Open syscall from: " << comm_id << endl;

   //extract the flags out of the end of the 
   //of the path
   int flags_index = strlen((char *)buf) + 1;
   int *flag_ptr = (int *)&(buf[flags_index]);
   int flags = flag_ptr[0];

   //actually do the open call
   int ret = open((char *)buf, flags);

   cout << "path: " << (char *)buf << endl;
   cout << "flags: " << flags << endl;
   cout << "ret: " << ret << endl;

   UInt8 reply[1024];
   int *reply_int = (int *)reply;
   reply_int[0] = ret;

   pt_endpt.ptMCPSend(comm_id,reply,sizeof(int));
}


