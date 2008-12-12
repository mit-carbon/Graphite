#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "syscall_server.h"
#include "sys/syscall.h"


using namespace std;

SyscallServer::SyscallServer(Network & network, 
      UnstructuredBuffer & send_buff_, UnstructuredBuffer &recv_buff_,
      const UInt32 SERVER_MAX_BUFF,
      char *scratch_)
: 
   _network(network),
   send_buff(send_buff_),
   recv_buff(recv_buff_),
   SYSCALL_SERVER_MAX_BUFF(SERVER_MAX_BUFF),
   scratch(scratch_)
{
}

SyscallServer::~SyscallServer()
{
}


void SyscallServer::handleSyscall(int comm_id)
{
   UInt8 syscall_number;
   recv_buff >> syscall_number;   

   switch(syscall_number)
   {
      case SYS_open:
         marshallOpenCall(comm_id);
         break;
      case SYS_read:
         marshallReadCall(comm_id);
         break;
      case SYS_write:
         marshallWriteCall(comm_id);
         break;
      case SYS_close:
         marshallCloseCall(comm_id);
         break;
      case SYS_access:
         marshallAccessCall(comm_id);
         break;
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

   // cerr << "Open syscall from: " << comm_id << endl;

   UInt32 len_fname;
   char *path = (char *) scratch;
   int flags;      

   recv_buff >> len_fname;

   if ( len_fname > SYSCALL_SERVER_MAX_BUFF )
      path = new char[len_fname];

   recv_buff >> make_pair(path, len_fname) >> flags;

   // Actually do the open call
   int ret = open(path, flags);

   //cerr << "len: " << len_fname << endl;
   //cerr << "path: " << path << endl;
   //cerr << "flags: " << flags << endl;
   //cerr << "ret: " << ret << endl;

   send_buff << ret;

   _network.netMCPSend(comm_id, send_buff.getBuffer(), send_buff.size());

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

   // cerr << "Read syscall from: " << comm_id << endl;

   int fd;
   char *buf = (char *) scratch;
   size_t count;
   char *dest;

   //create a temporary int for storing the addr
   int d2;
   recv_buff >> fd >> count >> d2;
   dest = (char *)d2;

   if ( count > SYSCALL_SERVER_MAX_BUFF )
      buf = new char[count];

   // Actually do the read call
   int bytes = read(fd, (void *) buf, count);  

   // Copy the memory into shared mem
   _network.getCore()->dcacheRunModel(Core::STORE, (ADDRINT)dest, buf, count);

   //cerr << "fd: " << fd << endl;
   //cerr << "buf: " << buf << endl;
   //cerr << "count: " << count << endl;
   //cerr << "bytes: " << bytes << endl;
   
   send_buff << bytes;
   if ( bytes != -1 )
     send_buff << make_pair(buf, bytes);

   _network.netMCPSend(comm_id, send_buff.getBuffer(), send_buff.size());   

   if ( count > SYSCALL_SERVER_MAX_BUFF )
      delete[] buf;

}


void SyscallServer::marshallWriteCall(int comm_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int
       COUNT               size_t
       BUFFER              char[]

       Transmit
       
       Field               Type
       -----------------|--------
       STATUS              int      

   */   

   // cerr << "Write syscall from: " << comm_id << endl;

   int fd;
   char *buf = (char *) scratch;
   size_t count;

   recv_buff >> fd >> count;
   
   if ( count > SYSCALL_SERVER_MAX_BUFF )
      buf = new char[count];

   // Previously we would pass the actual data, now we pass the
   // address and the syscall server will pull the data from the
   // shared memory. --cg3
   //recv_buff >> make_pair(buf, count);
   char *src;
   int src_b;
   recv_buff >> src_b;
   src = (char *)src_b;

   _network.getCore()->dcacheRunModel(Core::LOAD, (ADDRINT)src, buf, count);

   // Actually do the write call
   int bytes = write(fd, (void *) buf, count);  
   if ( bytes != -1 )
      cerr << "wrote: " << buf << endl;

   //cerr << "fd: " << fd << endl;
   //cerr << "buf: " << buf << endl;
   //cerr << "count: " << count << endl;
   //cerr << "bytes: " << bytes << endl;
   
   send_buff << bytes;

   _network.netMCPSend(comm_id, send_buff.getBuffer(), send_buff.size());   

   if ( count > SYSCALL_SERVER_MAX_BUFF )
      delete[] buf;

}


void SyscallServer::marshallCloseCall(int comm_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int

       Transmit
       
       Field               Type
       -----------------|--------
       STATUS              int      

   */   

   // cerr << "Close syscall from: " << comm_id << endl;

   int fd;
   recv_buff >> fd;
   
   // Actually do the close call
   int status = close(fd);  

   //cerr << "fd: " << fd << endl;
   //cerr << "status: " << status << endl;
   
   send_buff << status;
   _network.netMCPSend(comm_id, send_buff.getBuffer(), send_buff.size());   

}

void SyscallServer::marshallAccessCall(int comm_id)
{
   // cerr << "access syscall from: " << comm_id << endl;

   UInt32 len_fname;
   char *path = (char *) scratch;
   int mode;      

   recv_buff >> len_fname;

   if ( len_fname > SYSCALL_SERVER_MAX_BUFF )
      path = new char[len_fname];

   recv_buff >> make_pair(path, len_fname) >> mode;

   // Actually do the open call
   int ret = access(path, mode);

   //cerr << "len: " << len_fname << endl;
   //cerr << "path: " << path << endl;
   //cerr << "mode: " << mode << endl;
   //cerr << "ret: " << ret << endl;

   send_buff << ret;

   _network.netMCPSend(comm_id, send_buff.getBuffer(), send_buff.size());

   if ( len_fname > SYSCALL_SERVER_MAX_BUFF )
      delete[] path;
}
