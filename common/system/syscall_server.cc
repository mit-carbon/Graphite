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
   : m_network(network)
   , m_send_buff(send_buff_)
   , m_recv_buff(recv_buff_)
   , m_SYSCALL_SERVER_MAX_BUFF(SERVER_MAX_BUFF)
   , m_scratch(scratch_)
{
}

SyscallServer::~SyscallServer()
{
}


void SyscallServer::handleSyscall(core_id_t core_id)
{
   IntPtr syscall_number;
   m_recv_buff >> syscall_number;

   LOG_PRINT("Syscall: %i from core(%i, %i)", syscall_number, core_id.tile_id, core_id.core_type);

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

   case SYS_stat:
   case SYS_lstat:
      // Same as stat() except for a link
      marshallStatCall(syscall_number, core_id);
      break;

   case SYS_fstat:
      marshallFstatCall(core_id);
      break;

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

   case SYS_munmap:
      marshallMunmapCall (core_id);
      break;

   case SYS_brk:
      marshallBrkCall (core_id);
      break;
      
   case SYS_futex:
      marshallFutexCall (core_id);
      break;

   case SYS_rmdir:
      marshallRmdirCall (core_id);
      break;

   case SYS_unlink:
      marshallUnlinkCall (core_id);
      break;

   case SYS_getcwd:
      marshallGetCwdCall (core_id);
      break;

   case SYS_sched_getaffinity:
      marshallSchedGetAffinityCall (core_id);
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

void SyscallServer::marshallRmdirCall(core_id_t core_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       LEN_FNAME           UInt32
       FILE_NAME           char[]

       Transmit

       Field               Type
       -----------------|--------
       STATUS              int

   */

   UInt32 len_fname;
   char *path = (char *) m_scratch;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname);

   // Actually do rmdir open call
   int ret = syscall(SYS_rmdir, path);

   m_send_buff << ret;

   LOG_PRINT("Rmdir(%s) returns %i", path, ret);
   
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

void SyscallServer::marshallGetCwdCall(core_id_t core_id)
{
   /*
       Receive

       Field               Type
       -----------------|--------
       SIZE               size_t

       Transmit

       Field               Type
       -----------------|--------
       BYTES               int
       BUFFER              void *

   */
   char *read_buf = (char *) m_scratch;
   size_t size;

   assert(m_recv_buff.size() == (sizeof(size)));
   m_recv_buff >> size;

   if (size > m_SYSCALL_SERVER_MAX_BUFF)
      read_buf = new char[size];

   // Actually do the read call
   int bytes = syscall(SYS_getcwd, read_buf, size);

   m_send_buff << bytes;
   if (bytes != -1)
      m_send_buff << read_buf;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (size > m_SYSCALL_SERVER_MAX_BUFF)
      delete [] read_buf;
}

void SyscallServer::marshallUnlinkCall(core_id_t core_id)
{

   /*
       Receive

       Field               Type
       -----------------|--------
       LEN_FNAME           UInt32
       FILE_NAME           char[]

       Transmit

       Field               Type
       -----------------|--------
       STATUS              int

   */

   UInt32 len_fname;
   char *path = (char *) m_scratch;

   m_recv_buff >> len_fname;

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      path = new char[len_fname];

   m_recv_buff >> make_pair(path, len_fname);

   // Actually do the unlink call
   int ret = syscall(SYS_unlink, path);

   m_send_buff << ret;

   LOG_PRINT("Unlink(%s) returns %i", path, ret);
   
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   if (len_fname > m_SYSCALL_SERVER_MAX_BUFF)
      delete[] path;
}

void SyscallServer::marshallSchedSetAffinityCall(core_id_t core_id)
{
   /*
       Receive

       Field               Type
       -----------------|--------
       PID               pid_t
       CPUSETSIZE        unsigned int
       BUFFER            char[]

       Transmit

       Field               Type
       -----------------|--------
       STATUS              int
   */

   pid_t pid;
   unsigned int cpusetsize;
   int status;

   m_recv_buff >> pid >> cpusetsize;


   char *buf = new char [CPU_ALLOC_SIZE(cpusetsize)];
   m_recv_buff >> buf;
   cpu_set_t * mask = CPU_ALLOC(cpusetsize);
   *mask = *(cpu_set_t *) buf;

   status = Sim()->getThreadManager()->setThreadAffinity(pid, mask);

   m_send_buff << status;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   CPU_FREE(mask);
}



void SyscallServer::marshallSchedGetAffinityCall(core_id_t core_id)
{
   /*
       Receive

       Field               Type
       -----------------|--------
       PID               pid_t
       CPUSETSIZE        unsigned int

       Transmit

       Field               Type
       -----------------|--------
       STATUS              int
       BUFFER              void *

   */

   pid_t pid;
   unsigned int cpusetsize;
   int status;

   m_recv_buff >> pid >> cpusetsize;
   cpu_set_t *mask = CPU_ALLOC(cpusetsize);

   status = Sim()->getThreadManager()->getThreadAffinity(pid, mask);

   m_send_buff << status << mask;

   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   CPU_FREE(mask);
}


void SyscallServer::marshallFutexCall(core_id_t core_id)
{
   int *addr1;
   int op;
   int val1;
   void *timeout;
   int *addr2;
   int val3;

   UInt64 curr_time;

   m_recv_buff.get(addr1);
   m_recv_buff.get(op);
   m_recv_buff.get(val1);
   m_recv_buff.get(timeout);
   m_recv_buff.get(addr2);
   m_recv_buff.get(val3);
   
   m_recv_buff.get(curr_time);

   LOG_PRINT("Futex syscall: addr1(%p), op(%#x), val1(%i), timeout(%p), addr2(%p), val3(%i)",
              addr1, op, val1, timeout, addr2, val3);

   // Right now, we handle only a subset of the functionality
   // Trigger an assertion if something outside the subset occurs

#ifdef KERNEL_SQUEEZE
   LOG_ASSERT_ERROR( (op == FUTEX_WAIT)         ||    (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG))          ||
                     (op == FUTEX_WAKE)         ||    (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG))          ||
                     (op == FUTEX_WAKE_OP)      ||    (op == (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG))       ||
                     (op == FUTEX_CMP_REQUEUE)  ||    (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG))   ||
                     (op == (FUTEX_CLOCK_REALTIME | FUTEX_PRIVATE_FLAG | FUTEX_WAIT_BITSET)),
                     "op = %#x (Valid values are %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x)",
                     op,
                     FUTEX_WAIT,          FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
                     FUTEX_WAKE,          FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                     FUTEX_WAKE_OP,       FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG,
                     FUTEX_CMP_REQUEUE,   FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG,
                     FUTEX_CLOCK_REALTIME | FUTEX_PRIVATE_FLAG | FUTEX_WAIT_BITSET);
   
   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      LOG_ASSERT_ERROR(timeout == NULL, "timeout = %p", timeout);
   }
#endif

#ifdef KERNEL_LENNY
   LOG_ASSERT_ERROR( (op == FUTEX_WAIT)         ||    (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG))          ||
                     (op == FUTEX_WAKE)         ||    (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG))          ||
                     (op == FUTEX_WAKE_OP)      ||    (op == (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG))       ||
                     (op == FUTEX_CMP_REQUEUE)  ||    (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)),
                     "op = %#x (Valid values are %#x, %#x, %#x, %#x, %#x, %#x, %#x, %#x)",
                     op,
                     FUTEX_WAIT,          FUTEX_WAIT | FUTEX_PRIVATE_FLAG,
                     FUTEX_WAKE,          FUTEX_WAKE | FUTEX_PRIVATE_FLAG,
                     FUTEX_WAKE_OP,       FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG,
                     FUTEX_CMP_REQUEUE,   FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG);

   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      LOG_ASSERT_ERROR(timeout == NULL, "timeout = %p", timeout);
   }
#endif

#ifdef KERNEL_SQUEEZE
   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      futexWait(core_id, addr1, val1, curr_time); 
   }
   else if ((op == FUTEX_WAKE) || (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)))
   {
      futexWake(core_id, addr1, val1, curr_time);
   }
   else if ((op == FUTEX_WAKE_OP) || (op == (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)))
   {
      int val2 = (long int) timeout;
      futexWakeOp(core_id, addr1, val1, val2, addr2, val3, curr_time);
   }
   else if((op == FUTEX_CMP_REQUEUE) || (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)))
   {
      int val2 = (long int) timeout;
      futexCmpRequeue(core_id, addr1, val1, val2, addr2, val3, curr_time);
   }
   else if (op == (FUTEX_CLOCK_REALTIME | FUTEX_PRIVATE_FLAG | FUTEX_WAIT_BITSET))
   {
      futexWaitClockReal(core_id, addr1, val1, curr_time); 
   }
#endif

#ifdef KERNEL_LENNY
   if ((op == FUTEX_WAIT) || (op == (FUTEX_WAIT | FUTEX_PRIVATE_FLAG)))
   {
      futexWait(core_id, addr1, val1, curr_time); 
   }
   else if ((op == FUTEX_WAKE) || (op == (FUTEX_WAKE | FUTEX_PRIVATE_FLAG)))
   {
      futexWake(core_id, addr1, val1, curr_time);
   }
   else if ((op == FUTEX_WAKE_OP) || (op == (FUTEX_WAKE_OP | FUTEX_PRIVATE_FLAG)))
   {
      int val2 = (long int) timeout;
      futexWakeOp(core_id, addr1, val1, val2, addr2, val3, curr_time);
   }
   else if((op == FUTEX_CMP_REQUEUE) || (op == (FUTEX_CMP_REQUEUE | FUTEX_PRIVATE_FLAG)))
   {
      int val2 = (long int) timeout;
      futexCmpRequeue(core_id, addr1, val1, val2, addr2, val3, curr_time);
   }
#endif
}

// -- Futex related functions --
void SyscallServer::futexWait(core_id_t core_id, int *addr, int val, UInt64 curr_time)
{
   SimFutex *sim_futex = &m_futexes[(IntPtr) addr];

   int curr_val;
   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
   {
      Core* core = m_network.getTile()->getCore();
      core->accessMemory(Core::NONE, Core::READ, (IntPtr) addr, (char*) &curr_val, sizeof(curr_val));
   }
   else // (Config::getSingleton()->getSimulationMode() == Config::LITE)
   {
      curr_val = *addr;
   }

   LOG_PRINT("Futex Wait: core_id(%i,%i), addr(%p), val(%i), curr_val(%i), time(%llu ps)",
             core_id.tile_id, core_id.core_type, addr, val, curr_val, curr_time);
   
   if (val != curr_val)
   {
      m_send_buff.clear();
      m_send_buff << - (int) EWOULDBLOCK;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
   else
   {
      sim_futex->enqueueWaiter(core_id);
   }
}

#ifdef KERNEL_SQUEEZE
void SyscallServer::futexWaitClockReal(core_id_t core_id, int *addr, int val, UInt64 curr_time)
{
   int curr_val;
   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
   {
      Core* core = m_network.getTile()->getCore();
      core->accessMemory(Core::NONE, Core::READ, (IntPtr) addr, (char*) &curr_val, sizeof(curr_val));
   }
   else // (Config::getSingleton()->getSimulationMode() == Config::LITE)
   {
      curr_val = *addr;
   }

   LOG_PRINT("Futex Wait Clock Real: core_id(%i,%i), addr(%p), val(%i), curr_val(%i), time(%llu ps)",
             core_id.tile_id, core_id.core_type, addr, val, curr_val, curr_time);
   
   if (val != curr_val)
   {
      m_send_buff.clear();
      m_send_buff << - (int) ENOSYS;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
   else
   {
      LOG_PRINT_ERROR ("FUTEX_CLOCK_REALTIME not supported\n");
   }
}
#endif

void SyscallServer::futexWake(core_id_t core_id, int *addr, int val, UInt64 curr_time)
{
   LOG_PRINT("Futex Wake: core_id(%i,%i), addr(%p), val(%i), curr_time(%llu ps)",
             core_id.tile_id, core_id.core_type, addr, val, curr_time);

   int num_procs_woken_up = __futexWake(addr, val, curr_time);
   
   m_send_buff.clear();
   m_send_buff << num_procs_woken_up;
   m_send_buff << curr_time;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::futexWakeOp(core_id_t core_id, int *addr1, int val1, int val2, int* addr2, int val3, UInt64 curr_time)
{
   int OP = (val3 >> 28) & 0xf;
   int CMP = (val3 >> 24) & 0xf;
   int OPARG = (val3 >> 12) & 0xfff;
   int CMPARG = (val3) & 0xfff;

   int num_procs_woken_up = 0;

   // Get the old value of addr2
   int oldval;
   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
   {
      Core* core = m_network.getTile()->getCore();
      core->accessMemory(Core::NONE, Core::READ, (IntPtr) addr2, (char*) &oldval, sizeof(oldval));
   }
   else // (Config::getSingleton()->getSimulationMode() == Config::LITE)
   {
      oldval = *addr2;
   }

   LOG_PRINT("Futex WakeOp: core_id(%i,%i), addr1(%p), val1(%i), val2(%i), "
             "addr2(%p), val3(%i), oldval(%i), curr_time(%llu ps)",
             core_id.tile_id, core_id.core_type, addr1, val1, val2, addr2, val3, oldval, curr_time);

   int newval = 0;
   switch (OP)
   {
   case FUTEX_OP_SET:
      newval = OPARG;
      break;

   case FUTEX_OP_ADD:
      newval = oldval + OPARG;
      break;

   case FUTEX_OP_OR:
      newval = oldval | OPARG;
      break;

   case FUTEX_OP_ANDN:
      newval = oldval & (~OPARG);
      break;

   case FUTEX_OP_XOR:
      newval = oldval ^ OPARG;
      break;

   default:
      LOG_PRINT_ERROR("Futex syscall: FUTEX_WAKE_OP: Unhandled OP(%i)", OP);
      break;
   }
   
   // Write the newval into addr2
   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
   {
      Core* core = m_network.getTile()->getCore();
      core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) addr2, (char*) &newval, sizeof(newval));
   }
   else // (Config::getSingleton()->getSimulationMode() == Config::LITE)
   {
      *addr2 = newval;
   }

   // Wake upto val1 threads waiting on the first futex
   num_procs_woken_up += __futexWake(addr1, val1, curr_time);

   bool condition = false;
   switch (CMP)
   {
   case FUTEX_OP_CMP_EQ:
      condition = (oldval == CMPARG);
      break;

   case FUTEX_OP_CMP_NE:
      condition = (oldval != CMPARG);
      break;

   case FUTEX_OP_CMP_LT:
      condition = (oldval < CMPARG);
      break;

   case FUTEX_OP_CMP_LE:
      condition = (oldval <= CMPARG);
      break;

   case FUTEX_OP_CMP_GT:
      condition = (oldval > CMPARG);
      break;

   case FUTEX_OP_CMP_GE:
      condition = (oldval >= CMPARG);
      break;

   default:
      LOG_PRINT_ERROR("Futex syscall: FUTEX_WAKE_OP: Unhandled CMP(%i)", CMP);
      break;
   }
   
   // Wake upto val2 threads waiting on the second futex if the condition is true
   if (condition)
      num_procs_woken_up += __futexWake(addr2, val2, curr_time);

   // Send reply to the requester
   m_send_buff.clear();
   m_send_buff << num_procs_woken_up;
   m_send_buff << curr_time;
   m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
}

void SyscallServer::futexCmpRequeue(core_id_t core_id, int *addr1, int val1, int val2, int *addr2, int val3, UInt64 curr_time)
{
   int curr_val;
   if (Config::getSingleton()->getSimulationMode() == Config::FULL)
   {
      Core* core = m_network.getTile()->getCore();
      core->accessMemory(Core::NONE, Core::READ, (IntPtr) addr1, (char*) &curr_val, sizeof(curr_val));
   }
   else // (Config::getSingleton()->getSimulationMode() == Config::LITE)
   {
      curr_val = *addr1;
   }

   LOG_PRINT("Futex CmpRequeue: core_id(%i,%i), addr1(%p), val1(%i), val2(%i), "
             "addr2(%p), val3(%i), curr_val(%i), curr_time(%llu ps)",
             core_id.tile_id, core_id.core_type, addr1, val1, val2, addr2, val3, curr_val, curr_time);

   if (val3 != curr_val)
   {
      m_send_buff.clear();
      m_send_buff << - (int) EWOULDBLOCK;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
   else // (val3 == curr_val)
   {
      SimFutex *sim_futex = &m_futexes[(IntPtr) addr1];
      int num_procs_woken_up_or_requeued = 0;

      num_procs_woken_up_or_requeued += __futexWake(addr1, val1, curr_time);
      
      SimFutex *requeue_futex = &m_futexes[(IntPtr) addr2];

      for (SInt32 i = 0; i < val2; i++)
      {
         // dequeueWaiter changes the thread state to
         // RUNNING, which is changed back to STALLED 
         // by enqueueWaiter. Since only the MCP uses this state
         // this should be okay. 
         core_id_t waiter = sim_futex->dequeueWaiter();
         if (waiter.tile_id == INVALID_TILE_ID)
            break;

         num_procs_woken_up_or_requeued ++;
         
         requeue_futex->enqueueWaiter(waiter);
      }

      m_send_buff.clear();
      m_send_buff << num_procs_woken_up_or_requeued;
      m_send_buff << curr_time;
      m_network.netSend(core_id, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
   }
}

int SyscallServer::__futexWake(int* addr, int val, UInt64 curr_time)
{
   LOG_PRINT("__Futex Wake: addr(%p), val(%i), curr_time(%llu ps)", addr, val, curr_time);

   SimFutex *sim_futex = &m_futexes[(IntPtr) addr];

   int num_procs_woken_up = 0;
   for (int i = 0; i < val; i++)
   {
      core_id_t waiter = sim_futex->dequeueWaiter();
      if (waiter.tile_id == INVALID_TILE_ID)
         break;

      num_procs_woken_up ++;

      m_send_buff.clear();
      m_send_buff << (int) 0;
      m_send_buff << (UInt64) curr_time;
      m_network.netSend(waiter, MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());
      LOG_PRINT("Woke Up (%i,%i)", waiter.tile_id, waiter.core_type);
   }

   return num_procs_woken_up;
}

// -- SimFutex -- //
SimFutex::SimFutex()
{}

SimFutex::~SimFutex()
{
   if (!m_waiting.empty())
   {
      LOG_PRINT_WARNING("Waiting futexes still present at end of simulation");
      while (!m_waiting.empty())
      {
         core_id_t core_id = m_waiting.front();
         m_waiting.pop();
         LOG_PRINT_WARNING("Core (%i,%i) waiting", core_id.tile_id, core_id.core_type);
      }
   }
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
