#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "syscall_server.h"
#include "sys/syscall.h"



// Thread function
void* server_thread(void *dummy);


SyscallServer::SyscallServer() : SYSCALL_SERVER_MAX_BUFF(256*1024)
{
   pt_endpt.ptInitMCP();
   scratch = new char[SYSCALL_SERVER_MAX_BUFF];
}

SyscallServer::~SyscallServer()
{
   delete[] scratch;
}

void SyscallServer::run()
{
   cerr << "Waiting for syscall request..." << endl;

   send_buff.clear();
   recv_buff.clear();

   UInt32 length = 0;
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
      default:
	 cerr << "Unhandled MCP message type: " << msg_type << " from: " << comm_id << endl;
   }

   cerr << "Finished syscall request" << endl;
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
         cerr << "Unhandled syscall number: " << (int)syscall_number << " from: " << comm_id << endl;
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

   cerr << "Open syscall from: " << comm_id << endl;

   bool res;
   UInt32 len_fname;
   char *path = (char *) scratch;
   int flags;      

   res = recv_buff.get(len_fname);
   assert( res == true );

   if ( len_fname > SYSCALL_SERVER_MAX_BUFF )
      path = new char[len_fname];
   res = recv_buff.get((UInt8 *) path, len_fname);
   assert( res == true );
   
   res = recv_buff.get(flags);
   assert( res == true );

   // Actually do the open call
   int ret = open(path, flags);

   cerr << "path: " << path << endl;
   cerr << "flags: " << flags << endl;
   cerr << "ret: " << ret << endl;

   send_buff.put(ret);

   pt_endpt.ptMCPSend(comm_id, (UInt8 *) send_buff.getBuffer(), send_buff.size());

   if ( len_fname > SYSCALL_SERVER_MAX_BUFF )
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

   cerr << "Read syscall from: " << comm_id << endl;

   int fd;
   char *buf = (char *) scratch;
   size_t count;

   bool res = recv_buff.get(fd);
   assert( res == true );
   
   res = recv_buff.get(count);
   assert( res == true );

   if ( count > SYSCALL_SERVER_MAX_BUFF )
      buf = new char[count];

   // Actually do the read call
   int bytes = read(fd, (void *) buf, count);  

   cerr << "fd: " << fd << endl;
   cerr << "buf: " << buf << endl;
   cerr << "count: " << count << endl;
   cerr << "bytes: " << bytes << endl;
   
   send_buff.put(bytes);
   if ( bytes != -1 )
      send_buff.put((UInt8 *) buf, bytes);

   pt_endpt.ptMCPSend(comm_id, (UInt8 *) send_buff.getBuffer(), send_buff.size());   

   if ( count > SYSCALL_SERVER_MAX_BUFF )
      delete[] buf;

}
