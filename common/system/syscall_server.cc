// --- included for syscall: fstat
#include <sys/stat.h>

// ---- included for syscall: ioctl
#include <sys/ioctl.h>
#include <termios.h>

#include <sys/types.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#include "syscall_server.h"
#include "sys/syscall.h"
#include "tile.h"
#include "config.h"
#include "vm_manager.h"
#include "mcp.h"
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
   IntPtr syscall_number;
   m_recv_buff >> syscall_number;

   LOG_PRINT("Syscall: %d from core(%i, %i)", syscall_number, core_id.first, core_id.second);

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

   case SYS_writev:
      marshallWritevCall(core_id);
      break;

   case SYS_close:
      marshallCloseCall(core_id);
      break;

   case SYS_lseek:
      marshallLseekCall(core_id);
      break;

   case SYS_access:
      marshallAccessCall(core_id);
      break;

#ifdef TARGET_X86_64
   case SYS_stat:
   case SYS_lstat:
      // Same as stat() except for a link
      marshallStatCall(syscall_number, core_id);
      break;

   case SYS_fstat:
      marshallFstatCall(core_id);
      break;

#endif
#ifdef TARGET_IA32
   case SYS_fstat64:
      marshallFstat64Call(core_id);
      break;
#endif
   case SYS_ioctl:
      marshallIoctlCall(core_id);
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

#ifdef TARGET_IA32
   case SYS_mmap2:
      marshallMmap2Call(core_id);
      break;
#endif

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
      break;
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
       MODE                UInt64

       Transmit

       Field               Type
       -----------------|--------
       STATUS              int

   */

   UInt32 len_fname;
   char *path = (char *) m_scratch;
   int flags;
   UInt64 mode;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname) >> flags >> mode;

   // Actually do the open call
   int ret = syscall(SYS_open, path, flags, mode);

   m_send_buff << ret;

   LOG_PRINT("Open(%s,%i) returns %i", path, flags, ret);
   
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
   char *read_buf = (char *) m_scratch;
   size_t count;

   assert(m_recv_buff.size() == (sizeof(fd) + sizeof(count)));
   m_recv_buff >> fd >> count;

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      read_buf = new char[count];

   // Actually do the read call
   int bytes = syscall(SYS_read, fd, (void *) read_buf, count);

   m_send_buff << bytes;
   if (bytes != -1)
      m_send_buff << make_pair(read_buf, bytes);

   LOG_PRINT("Read(%i,%i) returns %i", fd, count, bytes);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete [] read_buf;
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
   int bytes = syscall(SYS_write, fd, (void *) buf, count);

   m_send_buff << bytes;

   LOG_PRINT("Write(%i,%i) returns %i", fd, count, bytes);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (count > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] buf;

}

void SyscallServer::marshallWritevCall(core_id_t core_id)
{
   //
   // Receive
   //
   // Field               Type
   // ------------------|---------
   // FILE DESCRIPTOR     int
   // COUNT               UInt64
   // BUFFER              char[]
   //
   // Transmit
   //
   // Field               Type
   // ------------------|---------
   // BYTES               IntPtr

   int fd;
   UInt64 count;
   char *buf = (char*) m_scratch;

   m_recv_buff >> fd >> count;

   if(count > m_SYSCALL_SERVER_MAX_BUFF)
      buf = new char[count];

   m_recv_buff >> make_pair(buf, count);

   // Write data to the file
   // Since we have already gathered data from all the various iovec's 
   // passed to the writev syscall, this is just a write syscall
   IntPtr bytes = syscall(SYS_write, fd, (void*) buf, count);

   m_send_buff << bytes;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if(count > m_SYSCALL_SERVER_MAX_BUFF)
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
   int status = syscall(SYS_close, fd);

   m_send_buff << status;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

}

void SyscallServer::marshallLseekCall(core_id_t core_id)
{
   int fd;
   off_t offset;
   int whence;
   m_recv_buff >> fd >> offset >> whence;

   // Actually do the lseek call
   off_t ret_val = syscall(SYS_lseek, fd, offset, whence);

   m_send_buff << ret_val;
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
   int ret = syscall(SYS_access, path, mode);

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

#ifdef TARGET_X86_64
void SyscallServer::marshallStatCall(IntPtr syscall_number, core_id_t core_id)
{
   char *path = (char *) m_scratch;
   struct stat stat_buf;

   UInt32 len_fname;
   // unpack the data

   m_recv_buff >> len_fname;
   
   assert(m_recv_buff.size() == ((SInt32) (len_fname + sizeof(struct stat))));

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname);
   m_recv_buff >> make_pair(&stat_buf, sizeof(struct stat));

   // Do the syscall
   int ret = syscall(syscall_number, path, &stat_buf);

   // pack the data and send
   m_send_buff.put<int>(ret);
   m_send_buff << make_pair(&stat_buf, sizeof(struct stat));

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   LOG_PRINT("Finished marshallStatCall(), path(%s), send_buf.size(%u)", path, m_send_buff.size());
}

void SyscallServer::marshallFstatCall(core_id_t core_id)
{
   int fd;
   struct stat buf;

   assert(m_recv_buff.size() == (sizeof(int) + sizeof(struct stat)));

   // unpack the data
   m_recv_buff.get<int>(fd);
   m_recv_buff >> make_pair(&buf, sizeof(struct stat));

   LOG_PRINT("In marshallFstatCall(), fd(%i), buf(%p)", fd, &buf);
   // Do the syscall
   int ret = syscall(SYS_fstat, fd, &buf);

   // pack the data and send
   m_send_buff.put<int>(ret);
   m_send_buff << make_pair(&buf, sizeof(struct stat));

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   LOG_PRINT("Finished marshallFstatCall(), fd(%i), buf(%p)", fd, &buf);
}
#endif

#ifdef TARGET_IA32
void SyscallServer::marshallFstat64Call(core_id_t core_id)
{
   int fd;
   struct stat64 buf;

   // unpack the data
   m_recv_buff.get<int>(fd);
   m_recv_buff >> make_pair(&buf, sizeof(struct stat64));

   // Do the syscall
   int ret = syscall(SYS_fstat64, fd, &buf);

   // pack the data and send
   m_send_buff.put<int>(ret);
   m_send_buff << make_pair(&buf, sizeof(struct stat64));

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}
#endif

void SyscallServer::marshallIoctlCall(core_id_t core_id)
{
   int fd;
   int request;
   struct termios buf;

   // unpack the data
   m_recv_buff.get<int>(fd);
   m_recv_buff.get<int>(request);
   m_recv_buff >> make_pair(&buf, sizeof(struct termios));

   // Do the syscall
   int ret = syscall(SYS_ioctl, fd, request, &buf);

   // pack the data and send
   m_send_buff.put<int>(ret);
   m_send_buff << make_pair(&buf, sizeof(struct termios));

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
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
   int ret = syscall(SYS_readahead, fd, offset, count);

   m_send_buff << ret;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallPipeCall(core_id_t core_id)
{
   int fds[2];

   // Actually do the pipe call
   int ret = syscall(SYS_pipe, fds);

   m_send_buff << ret;
   m_send_buff << fds[0] << fds[1];

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallMmapCall(core_id_t core_id)
{
   void *addr;
   size_t length;
   int prot;
   int flags;
   int fd;
   off_t pgoffset;

   m_recv_buff.get(addr);
   m_recv_buff.get(length);
   m_recv_buff.get(prot);
   m_recv_buff.get(flags);
   m_recv_buff.get(fd);
   m_recv_buff.get(pgoffset);

   void *start;
   start = Sim()->getMCP()->getVMManager()->mmap(addr, length, prot, flags, fd, pgoffset);

   m_send_buff.put(start);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

#ifdef TARGET_IA32
void SyscallServer::marshallMmap2Call(core_id_t core_id)
{
   void *addr;
   size_t length;
   int prot;
   int flags;
   int fd;
   off_t pgoffset;

   m_recv_buff.get(addr);
   m_recv_buff.get(length);
   m_recv_buff.get(prot);
   m_recv_buff.get(flags);
   m_recv_buff.get(fd);
   m_recv_buff.get(pgoffset);

   void *start;
   start = Sim()->getMCP()->getVMManager()->mmap2(addr, length, prot, flags, fd, pgoffset);

   m_send_buff.put(start);
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}
#endif

void SyscallServer::marshallMunmapCall (core_id_t core_id)
{
   void *addr;
   size_t length;

   m_recv_buff.get(addr);
   m_recv_buff.get(length);

   int ret_val;
   ret_val = Sim()->getMCP()->getVMManager()->munmap(addr, length);

   m_send_buff.put(ret_val);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::marshallBrkCall (core_id_t core_id)
{
   void *end_data_segment;

   m_recv_buff.get(end_data_segment);

   void *new_end_data_segment;
   new_end_data_segment = Sim()->getMCP()->getVMManager()->brk(end_data_segment);

   m_send_buff.put(new_end_data_segment);

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
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

   struct timespec timeout_buf;
   if (timeout_prefix == 0)
   {
      timeout = (struct timespec*) NULL;
   }
   else
   {
      assert(timeout_prefix == 1);
      timeout = &timeout_buf;
      m_recv_buff >> make_pair((char*) timeout, sizeof(struct timespec));
   }

   m_recv_buff.get(uaddr2);
   m_recv_buff.get(val3);

   m_recv_buff.get(curr_time);

   LOG_PRINT("Futex syscall: uaddr(0x%x), op(%u), val(%u)", uaddr, op, val);

   // Right now, we handle only a subset of the functionality
   // assert the subset

#ifdef KERNEL_LENNY
   LOG_ASSERT_ERROR((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)) \
            || (op == FUTEX_WAKE) || (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)) \
            || (op == FUTEX_CMP_REQUEUE) || (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)) \
            , "op = 0x%x", op);
   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      LOG_ASSERT_ERROR(timeout == NULL, "timeout = %p", timeout);
   }
#endif

#ifdef KERNEL_ETCH
   LOG_ASSERT_ERROR(((op == FUTEX_WAIT) || (op == FUTEX_WAKE)), "op = %u", op);
   if (op == FUTEX_WAIT)
   {
      LOG_ASSERT_ERROR(timeout == NULL, "timeout(%p)", timeout);
   }
#endif

   // Get the main core, assume for now PEP cores can't make syscalls.
   Core* core = m_network.getTile()->getCurrentCore();
   LOG_ASSERT_ERROR (core != NULL, "Core should not be NULL");
   int act_val;

   core->accessMemory(Core::NONE, Core::READ, (IntPtr) uaddr, (char*) &act_val, sizeof(act_val));

#ifdef KERNEL_LENNY
   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      futexWait(core_id, uaddr, val, act_val, curr_time); 
   }
   else if ((op == FUTEX_WAKE) || (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)))
   {
      futexWake(core_id, uaddr, val, curr_time);
   }
   else if((op == FUTEX_CMP_REQUEUE) || (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)))
   {
      futexCmpRequeue(core_id, uaddr, val, uaddr, val3, act_val, curr_time);
   }
#endif
   
#ifdef KERNEL_ETCH
   if (op == FUTEX_WAIT)
   {
      futexWait(core_id, uaddr, val, act_val, curr_time); 
   }
   else if (op == FUTEX_WAKE)
   {
      futexWake(core_id, uaddr, val, curr_time);
   }
#endif

}

// -- Futex related functions --
void SyscallServer::futexWait(core_id_t core_id, int *uaddr, int val, int act_val, UInt64 curr_time)
{
   LOG_PRINT("Futex Wait");
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
   LOG_PRINT("Futex Wake");
   SimFutex *sim_futex = &m_futexes[(IntPtr) uaddr];
   int num_procs_woken_up = 0;

   for (int i = 0; i < val; i++)
   {
      core_id_t waiter = sim_futex->dequeueWaiter();
      if (waiter.first == INVALID_TILE_ID)
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

void SyscallServer::futexCmpRequeue(core_id_t core_id, int *uaddr, int val, int *uaddr2, int val3, int act_val, UInt64 curr_time)
{
   LOG_PRINT("Futex CMP_REQUEUE");
   SimFutex *sim_futex = &m_futexes[(IntPtr) uaddr];
   int num_procs_woken_up = 0;

   if(val3 != act_val)
   {
      m_send_buff.clear();
      m_send_buff << (int) EAGAIN;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
   else
   {
      for(int i = 0; i < val; i++)
      {
         core_id_t waiter = sim_futex->dequeueWaiter();
         if(waiter.first == INVALID_TILE_ID)
            break;

         num_procs_woken_up++;
         
         m_send_buff.clear();
         m_send_buff << (int) 0;
         m_send_buff << (UInt64) curr_time;
         m_network.netSend(waiter, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
      }

      SimFutex *requeue_futex = &m_futexes[(IntPtr) uaddr2];

      while(true)
      {
         // dequeueWaiter changes the thread state to
         // RUNNING, which is changed back to STALLED 
         // by enqueueWaiter. Since only the MCP uses this state
         // this should be okay. 
         core_id_t waiter = sim_futex->dequeueWaiter();
         if(waiter.first == INVALID_TILE_ID)
            break;

         requeue_futex->enqueueWaiter(waiter);
      }

      m_send_buff.clear();
      m_send_buff << num_procs_woken_up;
      m_send_buff << (UInt64) curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }

   return;
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


