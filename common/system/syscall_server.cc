#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "syscall_server.h"
#include "sys/syscall.h"
#include "core.h"
#include "config.h"
#include "ocache.h"

#include "log.h"
#define LOG_DEFAULT_RANK m_network.getCore()->getId()
#define LOG_DEFAULT_MODULE SYSCALL

using namespace std;

SyscallServer::SyscallServer(Network & network,
                             UnstructuredBuffer & send_buff_, UnstructuredBuffer &recv_buff_,
                             const UInt32 SERVER_MAX_BUFF,
                             char *scratch_)
      :
      m_network(network),
      m_send_buff(send_buff_),
      m_recv_buff(recv_buff_),
      m_SYSCALL_SERVER_MAX_BUFF(SERVER_MAX_BUFF),
      m_scratch(scratch_)
{
}

SyscallServer::~SyscallServer()
{
}


void SyscallServer::handleSyscall(UInt32 core_id)
{
   UInt8 syscall_number;
   m_recv_buff >> syscall_number;

   switch (syscall_number)
   {
   case SYS_open:
      marshallOpenCall(core_id);
      break;
   case SYS_read:
      marshallReadCall(core_id);
      break;
   case SYS_write:
      marshallWriteCall(core_id);
      break;
   case SYS_close:
      marshallCloseCall(core_id);
      break;
   case SYS_access:
      marshallAccessCall(core_id);
      break;
   default:
      LOG_ASSERT_ERROR(false, "Unhandled syscall number: %i from %i", (int)syscall_number, core_id);
   }
}

void SyscallServer::marshallOpenCall(UInt32 core_id)
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

   // cerr << "Open syscall from: " << core_id << endl;

   UInt32 len_fname;
   char *path = (char *) m_scratch;
   int flags;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname) >> flags;

   // Actually do the open call
   int ret = open(path, flags);

   //cerr << "len: " << len_fname << endl;
   //cerr << "path: " << path << endl;
   //cerr << "flags: " << flags << endl;
   //cerr << "ret: " << ret << endl;

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

void SyscallServer::marshallReadCall(UInt32 core_id)
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

   // cerr << "Read syscall from: " << core_id << endl;

   int fd;
   char *buf = (char *) m_scratch;
   size_t count;
   char *dest;

   //create a temporary int for storing the addr
   int d2;
   m_recv_buff >> fd >> count >> d2;
   dest = (char *)d2;

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      buf = new char[count];

   // Actually do the read call
   int bytes = read(fd, (void *) buf, count);

   // Copy the memory into shared mem
   m_network.getCore()->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_STORE, (IntPtr)dest, buf, count);

   //cerr << "fd: " << fd << endl;
   //cerr << "buf: " << buf << endl;
   //cerr << "count: " << count << endl;
   //cerr << "bytes: " << bytes << endl;

   m_send_buff << bytes;
   if (bytes != -1)
      m_send_buff << make_pair(buf, bytes);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] buf;

}


void SyscallServer::marshallWriteCall(UInt32 core_id)
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

   // cerr << "Write syscall from: " << core_id << endl;

   int fd;
   char *buf = (char *) m_scratch;
   size_t count;

   m_recv_buff >> fd >> count;

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      buf = new char[count];

   // If we aren't using shared memory, then the data for the
   // write call must be passed in the message
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      char *src;
      int src_b;
      m_recv_buff >> src_b;
      src = (char *)src_b;

      m_network.getCore()->getOCache()->runDCacheModel(CacheBase::k_ACCESS_TYPE_LOAD, (IntPtr)src, buf, count);
   }
   else
   {
      m_recv_buff >> make_pair(buf, count);
   }

   // Actually do the write call
   int bytes = write(fd, (void *) buf, count);
//   if ( bytes != -1 )
//      cerr << "wrote: " << buf << endl;

   //cerr << "fd: " << fd << endl;
   //cerr << "buf: " << buf << endl;
   //cerr << "count: " << count << endl;
   //cerr << "bytes: " << bytes << endl;

   m_send_buff << bytes;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] buf;

}


void SyscallServer::marshallCloseCall(UInt32 core_id)
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

   // cerr << "Close syscall from: " << core_id << endl;

   int fd;
   m_recv_buff >> fd;

   // Actually do the close call
   int status = close(fd);

   //cerr << "fd: " << fd << endl;
   //cerr << "status: " << status << endl;

   m_send_buff << status;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

}

void SyscallServer::marshallAccessCall(UInt32 core_id)
{
   // cerr << "access syscall from: " << core_id << endl;

   UInt32 len_fname;
   char *path = (char *) m_scratch;
   int mode;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname) >> mode;

   // Actually do the open call
   int ret = access(path, mode);

   //cerr << "len: " << len_fname << endl;
   //cerr << "path: " << path << endl;
   //cerr << "mode: " << mode << endl;
   //cerr << "ret: " << ret << endl;

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}
