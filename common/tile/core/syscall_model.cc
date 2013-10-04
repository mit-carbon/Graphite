#include "syscall_model.h"
#include "message_types.h"
#include "core_model.h"
#include "sys/syscall.h"
#include "transport.h"
#include "config.h"

#include <cmath>
#include <cstring>

// --------------------------------------------
// New stuff added with Memory redirection

#include "simulator.h"
#include "tile.h"
#include "tile_manager.h"
#include "vm_manager.h"

#include <errno.h>
#include <string>

// --- included for syscall: fstat
#include <sys/stat.h>

// ---- included for syscall: ioctl
#include <sys/ioctl.h>
#include <termios.h>

// ----- Included for getrlimit / setrlimit 
#include <sys/time.h>
#include <sys/resource.h>

// ------ Included for rt_sigaction, rt_sigprocmask, rt_sigsuspend, sigreturn, kill
#include <signal.h>

// ------ Included for readahead
#include <fcntl.h>

// ------ Included for writev
#include <sys/uio.h>

using namespace std;

SyscallMdl::SyscallMdl(Core *core)
   : m_called_enter(false)
   , m_ret_val(0)
   , m_network(core->getTile()->getNetwork())
{
}

// --------------------------------------------
// New stuff added with Memory redirection

void SyscallMdl::saveSyscallNumber(IntPtr syscall_number)
{
   m_syscall_number = syscall_number;
}

IntPtr SyscallMdl::retrieveSyscallNumber()
{
   return m_syscall_number;
}

void SyscallMdl::saveSyscallArgs(syscall_args_t &args)
{
   m_saved_args.arg0 = args.arg0;
   m_saved_args.arg1 = args.arg1;
   m_saved_args.arg2 = args.arg2;
   m_saved_args.arg3 = args.arg3;
   m_saved_args.arg4 = args.arg4;
   m_saved_args.arg5 = args.arg5;
}

void SyscallMdl::retrieveSyscallArgs(syscall_args_t &args)
{
   args.arg0 = m_saved_args.arg0;
   args.arg1 = m_saved_args.arg1;
   args.arg2 = m_saved_args.arg2;
   args.arg3 = m_saved_args.arg3;
   args.arg4 = m_saved_args.arg4;
   args.arg5 = m_saved_args.arg5;
}

void* SyscallMdl::copyArgToBuffer(UInt32 arg_num, IntPtr arg_addr, UInt32 size)
{
   assert (arg_num < m_num_syscall_args);
   assert (size < m_scratchpad_size);
   char *scratchpad = m_scratchpad [arg_num];
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, arg_addr, scratchpad, size);
   return (void*) scratchpad;
}
   
void SyscallMdl::copyArgFromBuffer(UInt32 arg_num, IntPtr arg_addr, UInt32 size)
{
   assert (arg_num < m_num_syscall_args);
   assert (size < m_scratchpad_size);
   char *scratchpad = m_scratchpad[arg_num];
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory(Core::NONE, Core::WRITE, arg_addr, scratchpad, size);
}

// --------------------------------------------

IntPtr SyscallMdl::runExit(IntPtr old_return)
{
   if (m_called_enter)
   {
      m_called_enter = false;
      return m_ret_val;
   }
   else
   {
      return old_return;
   }
}

IntPtr SyscallMdl::runEnter(IntPtr syscall_number, syscall_args_t &args)
{
   LOG_PRINT("Got Syscall: %i", syscall_number);

   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_SYS_CALL;

   m_send_buff << msg_type << syscall_number;

   switch (syscall_number)
   {
   case SYS_open:
      m_called_enter = true;
      m_ret_val = marshallOpenCall(args);
      break;
   
   case SYS_read:
      m_called_enter = true;
      m_ret_val = marshallReadCall(args);
      break;

   case SYS_write:
      m_called_enter = true;
      m_ret_val = marshallWriteCall(args);
      break;

   case SYS_writev:
      m_called_enter = true;
      m_ret_val = marshallWritevCall(args);
      break;

   case SYS_close:
      m_called_enter = true;
      m_ret_val = marshallCloseCall(args);
      break;

   case SYS_lseek:
      m_called_enter = true;
      m_ret_val = marshallLseekCall(args);
      break;

   case SYS_access:
      m_called_enter = true;
      m_ret_val = marshallAccessCall(args);
      break;
  
   case SYS_stat:
   case SYS_lstat:
      // Same as stat() except that it stats a link
      m_called_enter = true;
      m_ret_val = marshallStatCall(args);
      break;

   case SYS_fstat:
      m_called_enter = true;
      m_ret_val = marshallFstatCall(args);
      break;

   case SYS_ioctl:
      m_called_enter = true;
      m_ret_val = marshallIoctlCall(args);
      break;

   case SYS_getpid:
      m_called_enter = true;
      m_ret_val = marshallGetpidCall (args);
      break;

   case SYS_readahead:
      m_called_enter = true;
      m_ret_val = marshallReadaheadCall (args);
      break;

   case SYS_pipe:
      m_called_enter = true;
      m_ret_val = marshallPipeCall (args);
      break;

   case SYS_mmap:
      m_called_enter = true;
      m_ret_val = marshallMmapCall (args);
      break;

   case SYS_munmap:
      m_called_enter = true;
      m_ret_val = marshallMunmapCall (args);
      break;

   case SYS_brk:
      m_called_enter = true;
      m_ret_val = marshallBrkCall (args);
      break;

   case SYS_futex:
      m_called_enter = true;
      m_ret_val = marshallFutexCall (args);
      break;

   case SYS_rmdir:
      m_called_enter = true;
      m_ret_val = marshallRmdirCall(args);
      break;

   case SYS_unlink:
      m_called_enter = true;
      m_ret_val = marshallUnlinkCall(args);
      break;

   case SYS_clock_gettime:
      m_called_enter = true;
      m_ret_val = handleClockGettimeCall(args);
      break;

   case SYS_getcwd:
      m_called_enter = true;
      m_ret_val = marshallGetCwdCall(args);
      break;

   case SYS_sched_setaffinity:
      m_called_enter = true;
      m_ret_val = marshallSchedSetAffinityCall(args);
      break;

   case SYS_sched_getaffinity:
      m_called_enter = true;
      m_ret_val = marshallSchedGetAffinityCall(args);
      break;

   case -1:
   default:
      break;
   }

   LOG_PRINT("Syscall finished");

   return m_called_enter ? SYS_getpid : syscall_number;
}

IntPtr SyscallMdl::marshallOpenCall(syscall_args_t &args)
{
   /*
       Syscall Args
       const char *pathname, int flags


       Transmit Protocol

       Field               Type
       -----------------|--------
       LEN_FNAME           UInt32
       FILE_NAME           char[]
       STATUS_FLAGS        int
       MODE                UInt64

       Receive Protocol

       Field               Type
       -----------------|--------
       STATUS              int

   */

   char *path = (char *)args.arg0;
   int flags = (int)args.arg1;
   UInt64 mode = (UInt64) args.arg2;

   UInt32 len_fname = getStrLen (path) + 1;
   
   char *path_buf = new char [len_fname];
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (IntPtr) path, (char*) path_buf, len_fname);


   m_send_buff << len_fname << make_pair(path_buf, len_fname) << flags << mode;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [] path_buf;
   delete [] (Byte*) recv_pkt.data;

   return status;
}


IntPtr SyscallMdl::marshallReadCall(syscall_args_t &args)
{

   /*
       Syscall Args
       int fd, void *buf, size_t count


       Transmit

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int
       COUNT               size_t

       Receive

       Field               Type
       -----------------|--------
       BYTES               int
       BUFFER              void *

   */

   int fd = (int)args.arg0;
   void *buf = (void *)args.arg1;
   size_t count = (size_t)args.arg2;
   Core *core = Sim()->getTileManager()->getCurrentCore();

   // if shared mem, provide the buf to read into
   m_send_buff << fd << count;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   assert(recv_pkt.length >= sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int bytes;
   m_recv_buff >> bytes;
   
   if (bytes != -1)
   {
      assert(m_recv_buff.size() == bytes);

      // Read data from MCP into a local buffer
      char* read_buf = new char[bytes];
      m_recv_buff >> make_pair(read_buf, bytes);
      
      // Write the data to memory
      Core* core = Sim()->getTileManager()->getCurrentCore();
      core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) buf, read_buf, bytes);
   }
   else
   {
      assert(m_recv_buff.size() == 0);
   }

   delete [] (Byte*) recv_pkt.data;

   return bytes;
}

IntPtr SyscallMdl::marshallWriteCall(syscall_args_t &args)
{
   /*
       Syscall Args
       int fd, void *buf, size_t count


       Transmit

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int
       COUNT               size_t
       BUFFER              char[]

       Receive

       Field               Type
       -----------------|--------
       BYTES               int

   */

   int fd = (int)args.arg0;
   void *buf = (void *)args.arg1;
   size_t count = (size_t)args.arg2;

   char *write_buf = new char [count];
   // Always pass all the data in the message, even if shared memory is available
   // I think this is a reasonable model and is definitely one less thing to keep
   // track of when you switch between shared-memory/no shared-memory
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (IntPtr) buf, (char*) write_buf, count);

   m_send_buff << fd << count << make_pair(write_buf, count);

   delete [] write_buf;

   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [] (Byte*) recv_pkt.data;

   return status;
}

IntPtr SyscallMdl::marshallWritevCall(syscall_args_t &args)
{
   //
   // Syscall Args
   // int fd, const struct iovec *iov, int iovcnt
   //
   // Transmit
   //
   // Field               Type
   // ------------------|---------
   // FILE DESCRIPTOR     int
   // COUNT               UInt64
   // BUFFER              char[]
   //
   // Receive
   //
   // Field               Type
   // ------------------|---------
   // BYTES               IntPtr

   int fd = (int) args.arg0;
   struct iovec *iov = (struct iovec*) args.arg1;
   int iovcnt = (int) args.arg2;

   Core *core = Sim()->getTileManager()->getCurrentCore();
   
   struct iovec *iov_buf = new struct iovec [iovcnt];
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) iov, (char*) iov_buf, iovcnt * sizeof (struct iovec));

   UInt64 count = 0;
   for (int i = 0; i < iovcnt; i++)
      count += iov_buf[i].iov_len;

   char *buf = new char[count];
   char* head = buf;
   int running_count = 0;
   
   for (int i = 0; i < iovcnt; i++)
   {
      core->accessMemory(Core::NONE, Core::READ, (IntPtr) iov_buf[i].iov_base, head, iov_buf[i].iov_len);
      running_count += iov_buf[i].iov_len;
      head = &buf[running_count];
   }

   m_send_buff << fd << count << make_pair(buf, count);

   delete [] buf;

   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(IntPtr));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   IntPtr status;
   m_recv_buff >> status;

   delete [] (Byte*) recv_pkt.data;

   return status;
}

IntPtr SyscallMdl::marshallCloseCall(syscall_args_t &args)
{
   /*
       Syscall Args
       int fd


       Transmit

       Field               Type
       -----------------|--------
       FILE_DESCRIPTOR     int

       Receive

       Field               Type
       -----------------|--------
       STATUS              int

   */

   int fd = (int)args.arg0;

   m_send_buff << fd;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [] (Byte*) recv_pkt.data;

   return status;
}

IntPtr SyscallMdl::marshallLseekCall(syscall_args_t &args)
{
   int fd = (int) args.arg0;
   off_t offset = (off_t) args.arg1;
   int whence = (int) args.arg2;

   m_send_buff << fd << offset << whence ;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   LOG_ASSERT_ERROR(recv_pkt.length == sizeof(off_t), "Recv Pkt length: expected(%u), got(%u)", sizeof(off_t), recv_pkt.length);
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   off_t ret_val;
   m_recv_buff >> ret_val;

   delete [] (Byte*) recv_pkt.data;

   return ret_val;
}

IntPtr SyscallMdl::marshallAccessCall(syscall_args_t &args)
{
   char *path = (char *)args.arg0;
   int mode = (int)args.arg1;

   UInt32 len_fname = getStrLen(path) + 1;
   char *path_buf = new char [len_fname];

   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (IntPtr) path, (char*) path_buf, len_fname);

   // pack the data
   m_send_buff << len_fname << make_pair(path_buf, len_fname) << mode;

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   m_recv_buff >> result;

   delete [] (Byte*) recv_pkt.data;
   delete [] path_buf;

   return result;
}

IntPtr SyscallMdl::marshallStatCall(syscall_args_t &args)
{
   char *path = (char*) args.arg0;
   struct stat stat_buf;

   UInt32 len_fname = getStrLen(path) + 1;
   char* path_buf = new char[len_fname]; 

   Core* core = Sim()->getTileManager()->getCurrentCore();
   // Read the data from memory
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) path, (char*) path_buf, len_fname);
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) args.arg1, (char*) &stat_buf, sizeof(struct stat));

   // pack the data
   m_send_buff << len_fname << make_pair(path_buf, len_fname);
   m_send_buff << make_pair(&stat_buf, sizeof(struct stat));

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get the result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   assert(m_recv_buff.size() == (sizeof(int) + sizeof(struct stat)));
   
   // Get the results
   int result;
   m_recv_buff.get<int>(result);
   m_recv_buff >> make_pair(&stat_buf, sizeof(struct stat));

   // Write the data to memory
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) args.arg1, (char*) &stat_buf, sizeof(struct stat));

   delete [] (Byte*) recv_pkt.data;
   delete [] path_buf;
   
   return result;
}

IntPtr SyscallMdl::marshallFstatCall(syscall_args_t &args)
{
   int fd = (int) args.arg0;
   struct stat buf;

   Core* core = Sim()->getTileManager()->getCurrentCore();
   // Read the data from memory
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) args.arg1, (char*) &buf, sizeof(struct stat));

   // pack the data
   m_send_buff.put<int>(fd);
   m_send_buff << make_pair(&buf, sizeof(struct stat));

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get the result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  
   assert(m_recv_buff.size() == (sizeof(int) + sizeof(struct stat)));

   // Get the results
   int result;
   m_recv_buff.get<int>(result);
   m_recv_buff >> make_pair(&buf, sizeof(struct stat));

   // Write the data to memory
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) args.arg1, (char*) &buf, sizeof(struct stat));

   delete [] (Byte*) recv_pkt.data;
   
   return result;
}

IntPtr SyscallMdl::marshallIoctlCall(syscall_args_t &args)
{
   int fd = (int) args.arg0;
   int request = (int) args.arg1;

   LOG_ASSERT_ERROR(request == TCGETS, "ioctl() system call, only TCGETS request supported, request(0x%x)", request);

   struct termios buf;

   Core* core = Sim()->getTileManager()->getCurrentCore();
   // Read the data from memory
   core->accessMemory(Core::NONE, Core::READ, (IntPtr) args.arg2, (char*) &buf, sizeof(struct termios));

   // pack the data
   m_send_buff.put<int>(fd);
   m_send_buff.put<int>(request);
   m_send_buff << make_pair(&buf, sizeof(struct termios));

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get the result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  
   // Get the results 
   int result;
   m_recv_buff.get<int>(result);
   m_recv_buff >> make_pair(&buf, sizeof(struct termios));

   // Write the data to memory
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) args.arg2, (char*) &buf, sizeof(struct termios));

   delete [] (Byte*) recv_pkt.data;
   
   return result;
}

IntPtr SyscallMdl::marshallGetpidCall (syscall_args_t &args)
{
   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   m_recv_buff >> result;

   delete [] (Byte*) recv_pkt.data;

   return result;
}

IntPtr SyscallMdl::marshallReadaheadCall(syscall_args_t &args)
{
   int fd = (int) args.arg0;
   UInt32 offset_msb = (UInt32) args.arg1;
   UInt32 offset_lsb = (UInt32) args.arg2;
   size_t count = (size_t) args.arg3;

   off64_t offset = offset_msb;
   offset = offset << 32;
   offset += offset_lsb;
   
   // pack the data
   m_send_buff << fd << offset << count;

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   m_recv_buff >> result;

   delete [] (Byte*) recv_pkt.data;

   return result;
}

IntPtr SyscallMdl::marshallPipeCall (syscall_args_t &args)
{
   int *fd = (int*) args.arg0;

   Core *core = Sim()->getTileManager()->getCurrentCore();

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   int fd_buff[2];

   m_recv_buff >> result;
   if (result == 0)
   {
      m_recv_buff >> fd_buff[0] >> fd_buff[1];
   }
   
   core->accessMemory (Core::NONE, Core::WRITE, (IntPtr) fd, (char*) fd_buff, 2 * sizeof(int));
      
   delete [] (Byte*) recv_pkt.data;

   return result;
}

IntPtr SyscallMdl::marshallMmapCall (syscall_args_t &args)
{
   // --------------------------------------------
   // Syscall arguments:
   //
   // struct mmap_arg_struct *args 
   //
   //  TRANSMIT
   //
   //  Field           Type
   //  --------------|------
   //  mmap_args_buf    mmap_arg_struct
   //
   //
   //  RECEIVE
   //
   //  Field           Type
   //  --------------|------
   //  start           void*
   // 
   // --------------------------------------------
   void *start = (void*) args.arg0;
   size_t length = (size_t) args.arg1;
   int prot = (int) args.arg2;
   int flags = (int) args.arg3;
   int fd = (int) args.arg4;
   off_t pgoffset = (off_t) args.arg5;

   // --------------------------------------------
   // Syscall arguments:
   //
   //  void *start, size_t length, int prot, int flags, int fd, off_t pgoffset
   //  TRANSMIT
   //
   //  Field           Type
   //  --------------|------
   //  start           void*
   //  length          size_t
   //  prot            int
   //  flags           int
   //  fd              int
   //  pgoffset        off_t
   //
   //
   //  RECEIVE
   //
   //  Field           Type
   //  --------------|------
   //  start           void*
   // 
   // --------------------------------------------



   LOG_PRINT("start(%p), length(0x%x), prot(0x%x), flags(0x%x), fd(%i), pgoffset(%u)",
         start, length, prot, flags, fd, pgoffset);

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put(start);
      m_send_buff.put(length);
      m_send_buff.put(prot);
      m_send_buff.put(flags);
      m_send_buff.put(fd);
      m_send_buff.put(pgoffset);

      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      // get a result
      NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
      recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      void *addr;
      m_recv_buff.get(addr);

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;

      return (carbon_reg_t) addr;
   }
   else
   {
      return (carbon_reg_t) syscall(SYS_mmap, start, length, prot, flags, fd, pgoffset);
   }
}

IntPtr SyscallMdl::marshallMunmapCall (syscall_args_t &args)
{
   // --------------------------------------------
   // Syscall arguments:
   //
   // struct mmap_arg_struct *args 
   //
   //  TRANSMIT
   //
   //  Field           Type
   //  --------------|------
   //  start           void*
   //  length          size_t
   //
   //
   //  RECEIVE
   //
   //  Field           Type
   //  --------------|------
   //  ret_val         int 
   // 
   // --------------------------------------------


   void *start = (void*) args.arg0;
   size_t length = (size_t) args.arg1;

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (start);
      m_send_buff.put (length);

      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

      // get a result
      NetPacket recv_pkt;
      Core *core = Sim()->getTileManager()->getCurrentCore();
      recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      int ret_val;
      m_recv_buff.get(ret_val);

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;

      return (carbon_reg_t) ret_val;
   }
   else
   {
      return (carbon_reg_t) syscall (SYS_munmap, start, length);
   }
}

IntPtr SyscallMdl::marshallBrkCall (syscall_args_t &args)
{
   // --------------------------------------------
   // Syscall arguments:
   //
   //  TRANSMIT
   //
   //  Field               Type
   //  ------------------|------
   //  end_data_segment    void*
   //
   //
   //  RECEIVE
   //
   //  Field                        Type
   //  ---------------------------|------
   //  new_end_data_segment         void* 
   // 
   // --------------------------------------------

   void *end_data_segment = (void*) args.arg0;

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (end_data_segment);

      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      // get a result
      NetPacket recv_pkt;
      Core *core = Sim()->getTileManager()->getCurrentCore();
      recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      void *new_end_data_segment;
      m_recv_buff.get (new_end_data_segment);

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;

      return (carbon_reg_t) new_end_data_segment;
   }
   else
   {
      return (carbon_reg_t) syscall (SYS_brk, end_data_segment);
   }
}

IntPtr SyscallMdl::marshallFutexCall (syscall_args_t &args)
{
   int *addr1 = (int*) args.arg0;
   int op = (int) args.arg1;
   int val1 = (int) args.arg2;
   const struct timespec *timeout = (const struct timespec*) args.arg3;
   int *addr2 = (int*) args.arg4;
   int val3 = (int) args.arg5;

   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      Core *core = Sim()->getTileManager()->getCurrentCore();
      LOG_ASSERT_ERROR(core, "Core = ((NULL))");

      UInt64 start_time;
      UInt64 end_time;

      start_time = core->getModel()->getCurrTime().getTime();

      // Package the arguments for the syscall
      m_send_buff.put(addr1);
      m_send_buff.put(op);
      m_send_buff.put(val1);
      m_send_buff.put(timeout);
      m_send_buff.put(addr2);
      m_send_buff.put(val3);

      m_send_buff.put(start_time);

      // send the data
      m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      // Set the CoreState to 'STALLED'
      core->setState(Core::STALLED);

      // get a result
      NetPacket recv_pkt;
      recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

      // Set the CoreState to 'RUNNING'
      core->setState(Core::WAKING_UP);

      // Create a buffer out of the result
      m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

      // Return the result
      int ret_val;
      m_recv_buff.get(ret_val);
      m_recv_buff.get(end_time);

      // For FUTEX_WAKE, end_time = start_time
      // Look at common/system/syscall_server.cc for this
      if (end_time > start_time)
      {
         if (core->getModel())
         {
            Time time_elapsed = Time(end_time - start_time);
            core->getModel()->processDynamicInstruction(new SyncInstruction(time_elapsed));
         }
      }

      // Delete the data buffer
      delete [] (Byte*) recv_pkt.data;

      return (carbon_reg_t) ret_val;
   }
   else
   {
      return (carbon_reg_t) syscall (SYS_futex, addr1, op, val1, timeout, addr2, val3);
   }
}

IntPtr SyscallMdl::marshallUnlinkCall(syscall_args_t &args)
{
   /*
       Syscall Args
       const char *pathname


       Transmit Protocol

       Field               Type
       -----------------|--------
       LEN_FNAME           UInt32
       FILE_NAME           char[]

       Receive Protocol

       Field               Type
       -----------------|--------
       STATUS              int

   */

   char *path = (char *)args.arg0;

   UInt32 len_fname = getStrLen (path) + 1;
   
   char *path_buf = new char [len_fname];
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (IntPtr) path, (char*) path_buf, len_fname);

   m_send_buff << len_fname << make_pair(path_buf, len_fname);
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [] path_buf;
   delete [] (Byte*) recv_pkt.data;

   return status;
}

IntPtr SyscallMdl::marshallRmdirCall(syscall_args_t &args)
{
  return(this->marshallUnlinkCall(args));
}

IntPtr SyscallMdl::handleClockGettimeCall(syscall_args_t &args)
{
   /* Notes
      (0) This syscall is handled locally rather than marshalling
          over to the MCP to handle it.  The reason is that
          for purposes of measuring elapsed time we want to be
          able to do that relative to *this* core, not the master
      (1) CoreModel is keeping a frequency re-normalized
          cycle count, so just using current cycle count and 
          frequency works fine to compute elapsed time.
      (2) if the core gets reset then 'time' also gets reset
   */

   clockid_t clk_id = (clockid_t ) args.arg0;
   struct timespec *ts = (struct timespec *) args.arg1;

   struct timespec temp_ts;
   UInt64 cycles = 0;
   double frequency = 0.0;
   double elapsed_time = 0.0;
   CoreModel* perf_model = 0L;

   if (clk_id != CLOCK_REALTIME) {
     /* we currently do not support anything but CLOCK_REALTIME */
     return -1;
   }

   Core* core = Sim()->getTileManager()->getCurrentCore();
   // compute the elapsed time
   perf_model = core->getModel();
   frequency = core->getFrequency();
   cycles = perf_model->getCurrTime().toCycles(frequency);
   elapsed_time = ((float) perf_model->getCurrTime().toNanosec())/1000000000.0;

   temp_ts.tv_sec = (time_t) floor(elapsed_time);
   temp_ts.tv_nsec = (long) ((elapsed_time - floor(elapsed_time))* 1000000000.0);

   // Write the data to memory
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) ts, (char*) (&temp_ts), sizeof(temp_ts));
   
   LOG_PRINT("clock_gettime called: cycles=%lu, frequency=%lf, elapse=%1.9lf, secs=%ld, nsecs=%ld\n",
             cycles, frequency, elapsed_time, (long) temp_ts.tv_sec, temp_ts.tv_nsec);

   return 0;
}

IntPtr SyscallMdl::marshallGetCwdCall(syscall_args_t &args)
{

   /*
       Syscall Args
       char* buf, size_t size

       Transmit

       Field               Type
       -----------------|--------
       SIZE               size_t

       Receive

       Field               Type
       -----------------|--------
       BYTES               int
       BUFFER              void *

   */

   char* buf = (char *)args.arg0;
   size_t size = (size_t)args.arg1;

   m_send_buff << size;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   assert(recv_pkt.length >= sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int bytes;
   m_recv_buff >> bytes;
   
   if (bytes != -1)
   {
      // Read data from MCP into a local buffer
      char* read_buf = new char[bytes];
      m_recv_buff >> read_buf;

      assert(strlen((const char*) read_buf) + 1 == (unsigned int) bytes);
   
      // Write the data to memory
      Core* core = Sim()->getTileManager()->getCurrentCore();
      core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) buf, read_buf, bytes);
   }
   else
   {
      assert(m_recv_buff.size() == 0);
   }

   delete [] (Byte*) recv_pkt.data;

   return (carbon_reg_t) buf;
}

IntPtr SyscallMdl::marshallSchedSetAffinityCall(syscall_args_t &args)
{
   assert(false);
   
   /*
       Syscall Args
       pid_t pid, unsigned int cpusetsize, cpu_set_t *mask

       Transmit

       Field               Type
       -----------------|--------
       PID               pid_t
       CPUSETSIZE        unsigned int
       BUFFER            char[]

       Receive

       Field               Type
       -----------------|--------
       STATUS              int

   */

   pid_t pid = (pid_t) args.arg0;
   unsigned int cpusetsize = (unsigned int) args.arg1;
   cpu_set_t* mask = (cpu_set_t*) args.arg2;
   int status = -1;

   char *write_buf = new char [CPU_ALLOC_SIZE(cpusetsize)];
   Core *core = Sim()->getTileManager()->getCurrentCore();
   core->accessMemory (Core::NONE, Core::READ, (IntPtr) mask, (char*) write_buf, CPU_ALLOC_SIZE(cpusetsize));

   m_send_buff << pid << cpusetsize << write_buf;
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
   m_recv_buff >> status;

   delete [] (Byte*) recv_pkt.data;
   delete [] write_buf;

   return status;

}
IntPtr SyscallMdl::marshallSchedGetAffinityCall(syscall_args_t &args)
{
   /*
       Syscall Args
       pid_t pid, unsigned int cpusetsize, cpu_set_t *mask

       Transmit

       Field               Type
       -----------------|--------
       PID               pid_t
       CPUSETSIZE        unsigned int

       Receive

       Field               Type
       -----------------|--------
       STATUS              int
       BUFFER              void *

   */

   pid_t pid = (pid_t) args.arg0;
   unsigned int cpusetsize = (unsigned int) args.arg1;
   cpu_set_t* mask = (cpu_set_t*) args.arg2;
   int status = -1;

   m_send_buff << pid << cpusetsize;
   Core *core = Sim()->getTileManager()->getCurrentCore();
   m_network->netSend(Config::getSingleton()->getMCPCoreId(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreId(), core->getId(), MCP_RESPONSE_TYPE);

   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   m_recv_buff >> status;

   // Read data from MCP into a local buffer
   char* read_buf = new char[CPU_ALLOC_SIZE(cpusetsize)];
   m_recv_buff >> read_buf;

   // Write the data to memory
   core->accessMemory(Core::NONE, Core::WRITE, (IntPtr) mask, read_buf, CPU_ALLOC_SIZE(cpusetsize));

   delete [] (Byte*) recv_pkt.data;
   delete [] read_buf;

   return status;
}

// Helper functions
UInt32 SyscallMdl::getStrLen (char *str)
{
   UInt32 len = 0;
   char c;
   char *ptr = str;
   while (1)
   {
      Core *core = Sim()->getTileManager()->getCurrentCore();
      core->accessMemory (Core::NONE, Core::READ, (IntPtr) ptr, &c, sizeof(char));
      if (c != '\0')
      {
         len++;
         ptr++;
      }
      else
         break;
   }
   return len;
}
