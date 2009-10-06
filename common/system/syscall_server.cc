#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "syscall_server.h"
#include "sys/syscall.h"
#include "core.h"
#include "config.h"
#include "vm_manager.h"
#include "simulator.h"
#include "thread_manager.h"

#include "log.h"

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


void SyscallServer::handleSyscall(core_id_t core_id)
{
   UInt8 syscall_number;
   m_recv_buff >> syscall_number;

   LOG_PRINT("Syscall: %d", syscall_number);

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
   case SYS_getpid:
      marshallGetpidCall(core_id);
      break;
   case SYS_readahead:
      marshallReadaheadCall(core_id);
      break;
   case SYS_pipe:
      marshallPipeCall(core_id);
      break;
   case SYS_mmap:
      marshallMmapCall(core_id);
      break;
   case SYS_mmap2:
      marshallMmap2Call(core_id);
      break;
   case SYS_munmap:
      marshallMunmapCall (core_id);
      break;
   case SYS_brk:
      marshallBrkCall (core_id);
      break;
   case SYS_futex:
      marshallFutexCall (core_id);
      break;
   default:
      LOG_ASSERT_ERROR(false, "Unhandled syscall number: %i from %i", (int)syscall_number, core_id);
   }

   LOG_PRINT("Finished syscall: %d", syscall_number);
}

void SyscallServer::marshallOpenCall(core_id_t core_id)
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

   UInt32 len_fname;
   char *path = (char *) m_scratch;
   int flags;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname) >> flags;

   // Actually do the open call
   int ret = open(path, flags);

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

void SyscallServer::marshallReadCall(core_id_t core_id)
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
   m_network.getCore()->accessMemory(Core::NONE, Core::WRITE, (IntPtr)dest, buf, count);

   m_send_buff << bytes;
   if (bytes != -1 && !Config::getSingleton()->isSimulatingSharedMemory())
      m_send_buff << make_pair(buf, bytes);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] buf;
}


void SyscallServer::marshallWriteCall(core_id_t core_id)
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

   int fd;
   char *buf = (char *) m_scratch;
   size_t count;

   m_recv_buff >> fd >> count;

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      buf = new char[count];

   // All data is always passed in the message, even if shared memory is available
   // I think this is a reasonable model and is definitely one less thing to keep
   // track of when you switch between shared-memory/no shared-memory
   m_recv_buff >> make_pair(buf, count);

   // Actually do the write call
   int bytes = write(fd, (void *) buf, count);

   m_send_buff << bytes;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] buf;

}


void SyscallServer::marshallCloseCall(core_id_t core_id)
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

   int fd;
   m_recv_buff >> fd;

   // Actually do the close call
   int status = close(fd);

   m_send_buff << status;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

}

void SyscallServer::marshallAccessCall(core_id_t core_id)
{
   UInt32 len_fname;
   char *path = (char *) m_scratch;
   int mode;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname) >> mode;

   // Actually do the open call
   int ret = access(path, mode);

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

void SyscallServer::marshallGetpidCall(core_id_t core_id)
{
   // Actually do the getpid call
   int ret = getpid();

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallReadaheadCall(core_id_t core_id)
{
   int fd;
   off64_t offset;
   size_t count;

   m_recv_buff >> fd >> offset >> count;

   // Actually do the readahead call
   int ret = readahead (fd, offset, count);

   m_send_buff << ret;

   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallPipeCall(core_id_t core_id)
{
   int fds[2];

   // Actually do the pipe call
   int ret = pipe (fds);

   m_send_buff << ret;
   m_send_buff << fds[0] << fds[1];

   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallMmapCall (core_id_t core_id)
{
   struct mmap_arg_struct mmap_args_buf;

   m_recv_buff.get(mmap_args_buf);

   void *start;
   start = VMManager::getSingleton()->mmap ( (void*) mmap_args_buf.addr,
         (size_t) mmap_args_buf.len,
         (int) mmap_args_buf.prot,
         (int) mmap_args_buf.flags,
         (int) mmap_args_buf.fd,
         (off_t) mmap_args_buf.offset);

   m_send_buff.put (start);

   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallMmap2Call (core_id_t core_id)
{
   void *start;
   size_t length;
   int prot;
   int flags;
   int fd;
   off_t pgoffset;

   m_recv_buff.get (start);
   m_recv_buff.get (length);
   m_recv_buff.get (prot);
   m_recv_buff.get (flags);
   m_recv_buff.get (fd);
   m_recv_buff.get (pgoffset);

   void *addr;
   addr = VMManager::getSingleton()->mmap2 (start, length, prot, flags, fd, pgoffset);

   m_send_buff.put (addr);
   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallMunmapCall (core_id_t core_id)
{
   void *start;
   size_t length;

   m_recv_buff.get(start);
   m_recv_buff.get(length);

   int ret_val;
   ret_val = VMManager::getSingleton()->munmap (start, length);

   m_send_buff.put (ret_val);

   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallBrkCall (core_id_t core_id)
{
   void *end_data_segment;

   m_recv_buff.get(end_data_segment);

   void *new_end_data_segment;
   new_end_data_segment = VMManager::getSingleton()->brk(end_data_segment);

   m_send_buff.put (new_end_data_segment);

   m_network.netSend (core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallFutexCall (core_id_t core_id)
{
   int *uaddr;
   int op;
   int val;
   struct timespec *timeout;
   int *uaddr2;
   int val3;

   int timeout_prefix;

   UInt64 curr_time;

   m_recv_buff.get(uaddr);
   m_recv_buff.get(op);
   m_recv_buff.get(val);

   m_recv_buff.get(timeout_prefix);

   if (timeout_prefix == 0)
   {
      timeout = (struct timespec*) NULL;
   }
   else
   {
      assert(timeout_prefix == 1);
      timeout = (struct timespec*) malloc(sizeof(struct timespec));
      m_recv_buff >> make_pair((void*) timeout, sizeof(*timeout));
   }

   m_recv_buff.get(uaddr2);
   m_recv_buff.get(val3);

   m_recv_buff.get(curr_time);

   // Right now, we handle only a subset of the functionality
   // assert the subset

   LOG_ASSERT_ERROR((op == FUTEX_WAIT) || (op == FUTEX_WAKE), "op = %u", op);
   if (op == FUTEX_WAIT)
   {
      LOG_ASSERT_ERROR(timeout == NULL, "timeout(%p)", timeout);
   }

   if (timeout != NULL)
   {
      free (timeout);
   }

   Core* core = m_network.getCore();
   LOG_ASSERT_ERROR (core != NULL, "Core should not be NULL");
   int act_val;

   core->accessMemory(Core::NONE, Core::READ, (IntPtr) uaddr, (char*) &act_val, sizeof(act_val));

   if (op == FUTEX_WAIT)
   {
      futexWait(core_id, uaddr, val, act_val, curr_time); 
   }
   else if (op == FUTEX_WAKE)
   {
      futexWake(core_id, uaddr, val, curr_time);
   }

}

// -- Futex related functions --
void SyscallServer::futexWait(core_id_t core_id, int *uaddr, int val, int act_val, UInt64 curr_time)
{
   SimFutex *sim_futex = &m_futexes[(IntPtr) uaddr];
  
   if (val != act_val)
   {
      m_send_buff.clear();
      m_send_buff << (int) EWOULDBLOCK;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
   else
   {
      sim_futex->enqueueWaiter(core_id);
   }
}

void SyscallServer::futexWake(core_id_t core_id, int *uaddr, int val, UInt64 curr_time)
{
   SimFutex *sim_futex = &m_futexes[(IntPtr) uaddr];
   int num_procs_woken_up = 0;

   for (int i = 0; i < val; i++)
   {
      core_id_t waiter = sim_futex->dequeueWaiter();
      if (waiter == INVALID_CORE_ID)
         break;

      num_procs_woken_up ++;

      m_send_buff.clear();
      m_send_buff << (int) 0;
      m_send_buff << (UInt64) curr_time;
      m_network.netSend(waiter, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }

   m_send_buff.clear();
   m_send_buff << num_procs_woken_up;
   m_send_buff << (UInt64) curr_time;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

}

// -- SimFutex -- //
SimFutex::SimFutex()
{}

SimFutex::~SimFutex()
{
   assert(m_waiting.empty());
}

void SimFutex::enqueueWaiter(core_id_t core_id)
{
   Sim()->getThreadManager()->stallThread(core_id);
   m_waiting.push(core_id);
}

core_id_t SimFutex::dequeueWaiter()
{
   if (m_waiting.empty())
      return INVALID_CORE_ID;
   else
   {
      core_id_t core_id = m_waiting.front();
      m_waiting.pop();

      Sim()->getThreadManager()->resumeThread(core_id);
      return core_id;
   }
}


