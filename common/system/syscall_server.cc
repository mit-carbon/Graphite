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
   send_buff.clear();
   recv_buff.clear();

   UInt32 length;
   UInt8* buf = pt_endpt.ptMCPRecv(&length); 
   recv_buff.put(buf, length);
  
   int msg_type;
   int comm_id;

   recv_buff.get(msg_type);
   recv_buff.get(comm_id);

   switch(msg_type)
   {
      case 0:
	 handleSyscall(comm_id);
         break;
   }
}

void SyscallServer::handleSyscall(int comm_id)
{
   UInt8 syscall_number;
   recv_buff.get(syscall_number);   

   switch(syscall_number)
   {
      case SYS_open:
      {
         marshallOpenCall(comm_id);
         break;
      }
      case SYS_read:
      {
	 marshallReadCall(comm_id);
         break;
      }
      default:
      {
         cout << "Unhandled syscall number: " << (int)syscall_number << " from: " << comm_id << endl;
      }
   }
}

void SyscallServer::marshallOpenCall(int comm_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       LEN_FNAME           UInt32
       FILE_NAME           char[]
       FLAGS               int

       Transmit
       
       Field               Type
       -----------------|--------
       STATUS              int       

   */   

   cout << "Open syscall from: " << comm_id << endl;

   bool res;
   UInt32 len_fname;
   char buff[256];
   char *path = (char *) buff;
   int flags;      

   res = recv_buff.get(len_fname);
   assert( res == true );

   if (len_fname > 256)
      path = new char[len_fname];
   res = recv_buff.get((UInt8 *) path, len_fname);
   assert( res == true );
   
   res = recv_buff.get(flags);
   assert( res == true );

   //actually do the open call
   int ret = open(path, flags);

   cout << "path: " << path << endl;
   cout << "flags: " << flags << endl;
   cout << "ret: " << ret << endl;

   send_buff.put(ret);

   pt_endpt.ptMCPSend(comm_id, (UInt8 *) send_buff.getBuffer(), send_buff.size());

   if ( len_fname > 256 )
      delete[] path;
}

void SyscallServer::marshallReadCall(int comm_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int
       COUNT               size_t

       Transmit
       
       Field               Type
       -----------------|--------
       STATUS              int
       BUFFER              void *      

   */   

   cout << "Read syscall from: " << comm_id << endl;

  
}
