#include "syscall_model.h"
#include "sys/syscall.h"
#include "transport.h"
#include "config.h"

// --------------------------------------------
// New stuff added with Memory redirection

#include "simulator.h"
#include "core.h"
#include "core_manager.h"
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

// ----------------------------------------------

using namespace std;

SyscallMdl::SyscallMdl(Network *net)
      : m_called_enter(false),
      m_ret_val(0),
      m_network(net)
{
}

// --------------------------------------------
// New stuff added with Memory redirection

void SyscallMdl::saveSyscallNumber (carbon_reg_t syscall_number)
{
   m_syscall_number = syscall_number;
}

carbon_reg_t SyscallMdl::retrieveSyscallNumber ()
{
   return m_syscall_number;
}

void SyscallMdl::saveSyscallArgs (syscall_args_t &args)
{
   m_saved_args.arg0 = args.arg0;
   m_saved_args.arg1 = args.arg1;
   m_saved_args.arg2 = args.arg2;
   m_saved_args.arg3 = args.arg3;
   m_saved_args.arg4 = args.arg4;
   m_saved_args.arg5 = args.arg5;
}

void SyscallMdl::retrieveSyscallArgs (syscall_args_t &args)
{
   args.arg0 = m_saved_args.arg0;
   args.arg1 = m_saved_args.arg1;
   args.arg2 = m_saved_args.arg2;
   args.arg3 = m_saved_args.arg3;
   args.arg4 = m_saved_args.arg4;
   args.arg5 = m_saved_args.arg5;
}

void* SyscallMdl::copyArgToBuffer (unsigned int arg_num, IntPtr arg_addr, unsigned int size)
{
   assert (arg_num < m_num_syscall_args);
   assert (size < m_scratchpad_size);
   char *scratchpad = m_scratchpad [arg_num];
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   core->accessMemory (Core::NONE, READ, arg_addr, scratchpad, size);
   return (void*) scratchpad;
}
   
void SyscallMdl::copyArgFromBuffer (unsigned int arg_num, IntPtr arg_addr, unsigned int size)
{
   assert (arg_num < m_num_syscall_args);
   assert (size < m_scratchpad_size);
   char *scratchpad = m_scratchpad [arg_num];
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   core->accessMemory (Core::NONE, READ, arg_addr, scratchpad, size);
}

// --------------------------------------------

carbon_reg_t SyscallMdl::runExit(int old_return)
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

UInt8 SyscallMdl::runEnter(UInt8 syscall_number, syscall_args_t &args)
{
   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_SYS_CALL;

   m_send_buff << msg_type << syscall_number;

   switch (syscall_number)
   {
      case SYS_open:
         {
            m_called_enter = true;
            m_ret_val = marshallOpenCall(args);
            break;
         }
      case SYS_read:
         {
            m_called_enter = true;
            m_ret_val = marshallReadCall(args);
            break;
         }

      case SYS_write:
         {
            m_called_enter = true;
            m_ret_val = marshallWriteCall(args);
            break;
         }
      case SYS_close:
         {
            m_called_enter = true;
            m_ret_val = marshallCloseCall(args);
            break;
         }
      case SYS_access:
         {
            m_called_enter = true;
            m_ret_val = marshallAccessCall(args);
            break;
         }

      case SYS_mmap:
         m_called_enter = true;
         m_ret_val = marshallMmapCall (args);
         break;

      case SYS_mmap2:
         m_called_enter = true;
         m_ret_val = marshallMmap2Call (args);
         break;
      
      case SYS_munmap:
         m_called_enter = true;
         m_ret_val = marshallMunmapCall (args);
         break;

      case SYS_brk:
         m_called_enter = true;
         m_ret_val = marshallBrkCall (args);
         break;

      case -1:
      default:
         break;
   }

   return m_called_enter ? SYS_getpid : syscall_number;

}

carbon_reg_t SyscallMdl::marshallOpenCall(syscall_args_t &args)
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

       Receive Protocol

       Field               Type
       -----------------|--------
       STATUS              int

   */

   char *path = (char *)args.arg0;
   int flags = (int)args.arg1;
   UInt32 len_fname = strlen(path) + 1;

   m_send_buff << len_fname << make_pair(path, len_fname) << flags;
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*)recv_pkt.data;

   return status;
}


carbon_reg_t SyscallMdl::marshallReadCall(syscall_args_t &args)
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


   // if shared mem, provide the buf to read into
   m_send_buff << fd << count << (int)buf;
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);

   assert(recv_pkt.length >= sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int bytes;
   m_recv_buff >> bytes;

   if (bytes != -1)
   {
      m_recv_buff >> make_pair(buf, bytes);
   }
   else
   {
      assert(m_recv_buff.size() == 0);
   }

   delete [](Byte*)recv_pkt.data;

   return bytes;
}

carbon_reg_t SyscallMdl::marshallWriteCall(syscall_args_t &args)
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

   // Always pass all the data in the message, even if shared memory is available
   // I think this is a reasonable model and is definitely one less thing to keep
   // track of when you switch between shared-memory/no shared-memory
   m_send_buff << fd << count << make_pair(buf, count);

   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*) recv_pkt.data;

   return status;
}

carbon_reg_t SyscallMdl::marshallCloseCall(syscall_args_t &args)
{
   /*
       Syscall Args
       int fd, void *buf, size_t count


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
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*) recv_pkt.data;

   return status;
}

carbon_reg_t SyscallMdl::marshallAccessCall(syscall_args_t &args)
{
   char *path = (char *)args.arg0;
   int mode = (int)args.arg1;
   UInt32 len_fname = strlen(path) + 1;

   // pack the data
   m_send_buff << len_fname << make_pair(path, len_fname) << mode;

   // send the data
   m_network->netSend(Config::getSingleton()->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   m_recv_buff >> result;

   delete [](Byte*) recv_pkt.data;

   return result;
}

carbon_reg_t SyscallMdl::marshallFstatCall (syscall_args_t &args)
{
   //------------------------------------------
   //  Syscall Arguments:
   //  int filedes, struct stat *buf
   //
   //  TRANSMIT
   //
   //  Field           Type
   //  --------------|------
   //  fd               int
   //
   //  RECEIVE
   //
   //  Field           Type
   //  --------------|------
   //  status          int
   //  buf             struct stat
   //
   //
   //  ------------------------------------------

   int fd = (int) args.arg0;
   struct stat *buf = (struct stat*) args.arg1;
   
   // pack the data
   m_send_buff << fd;

   // send the data
   m_network->netSend (Config::getSingleton()->getMCPCoreNum (), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum (), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   struct stat buffer;
   m_recv_buff >> result;
   m_recv_buff.get<struct stat> (buffer);

   Core *core = Sim()->getCoreManager()->getCurrentCore();
   // FIXME
   // Have to do this because of the weird difference in the size of struct stat between newlib and glibc
   core->accessMemory (Core::NONE, WRITE, (IntPtr) buf, (char*) &buffer, 64);
   
   return result;
}

carbon_reg_t SyscallMdl::marshallIoctlCall (syscall_args_t &args)
{
   // --------------------------------------------
   // Syscall arguments:
   // int fd, int request, struct termios *argp (we only support 'request == TCGETS')
   //
   //  TRANSMIT
   //
   //  Field           Type
   //  --------------|------
   //  fd               int
   //  request          int
   //
   //
   //  RECEIVE
   //
   //  Field           Type
   //  --------------|------
   //  ret             int
   //  argp_buf        struct termios (only if ret != -1)
   //
   //  ------------------------------------------
   
   int fd = (int) args.arg0;
   int request = (int) args.arg1;
   struct termios *argp = (struct termios*) args.arg2; 

   assert ( request == TCGETS );
   
   // pack the data
   m_send_buff << fd << request;

   // send the data
   m_network->netSend (Config::getSingleton()->getMCPCoreNum (), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum (), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

   // return the result
   int ret;
   struct termios argp_buf;
   m_recv_buff >> ret;
   if (ret != -1)
   {
      m_recv_buff.get<struct termios> (argp_buf);
      Core *core = Sim()->getCoreManager()->getCurrentCore();
      core->accessMemory (Core::NONE, WRITE, (IntPtr) argp, (char*) &argp_buf, sizeof (struct termios));
   }
   
   return ret;
}

carbon_reg_t SyscallMdl::marshallMprotectCall (syscall_args_t &args)
{
   // --------------------------------------------
   // Syscall arguments:
   //
   // const void *addr, size_t len, int prot
   //
   // Make the system call locally for now
   // In multi-machine simulations, we should be making the system call on the MCP
   // as well as making it locally, I think
   //
   // --------------------------------------------

   void *fd = (void*) args.arg0;
   size_t len = (size_t) args.arg1;
   int prot = (int) args.arg2;

   return (carbon_reg_t) syscall (SYS_mprotect, fd, len, prot);
}

carbon_reg_t SyscallMdl::marshallMmapCall (syscall_args_t &args)
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


   struct mmap_arg_struct *mmap_args_ptr = (struct mmap_arg_struct*) args.arg0;

#ifdef REDIRECT_MEMORY
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (*mmap_args_ptr);
      
      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreNum (), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

      // get a result
      NetPacket recv_pkt;
      recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum (), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      void *start;
      m_recv_buff.get(start);
      return (carbon_reg_t) start;
   }
   else
   {
#endif
      return (carbon_reg_t) syscall (SYS_mmap, mmap_args_ptr);
#ifdef REDIRECT_MEMORY
   }
#endif
}

carbon_reg_t SyscallMdl::marshallMmap2Call (syscall_args_t &args)
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


   struct mmap_arg_struct *mmap_args_ptr = (struct mmap_arg_struct*) args.arg0;

#ifdef REDIRECT_MEMORY
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (*mmap_args_ptr);
      
      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreNum (), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

      // get a result
      NetPacket recv_pkt;
      recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum (), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      void *start;
      m_recv_buff.get(start);
      return (carbon_reg_t) start;
   }
   else
   {
#endif
      return (carbon_reg_t) syscall (SYS_mmap2, mmap_args_ptr);
#ifdef REDIRECT_MEMORY
   }
#endif
}

carbon_reg_t SyscallMdl::marshallMunmapCall (syscall_args_t &args)
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

#ifdef REDIRECT_MEMORY
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (start);
      m_send_buff.put (length);

      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreNum (), MCP_REQUEST_TYPE, m_send_buff.getBuffer (), m_send_buff.size ());

      // get a result
      NetPacket recv_pkt;
      recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum (), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      int ret_val;
      m_recv_buff.get(ret_val);
      return (carbon_reg_t) ret_val;
   }
   else
   {
#endif
      return (carbon_reg_t) syscall (SYS_munmap, start, length);
#ifdef REDIRECT_MEMORY
   }
#endif
}

carbon_reg_t SyscallMdl::marshallBrkCall (syscall_args_t &args)
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

#ifdef REDIRECT_MEMORY
   if (Config::getSingleton()->isSimulatingSharedMemory())
   {
      m_send_buff.put (end_data_segment);

      // send the data
      m_network->netSend (Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

      // get a result
      NetPacket recv_pkt;
      recv_pkt = m_network->netRecv (Config::getSingleton()->getMCPCoreNum(), MCP_RESPONSE_TYPE);

      // Create a buffer out of the result
      m_recv_buff << make_pair (recv_pkt.data, recv_pkt.length);

      // Return the result
      void *new_end_data_segment;
      m_recv_buff.get (new_end_data_segment);
      return (carbon_reg_t) new_end_data_segment;
   }
   else
   {
#endif
      return (carbon_reg_t) syscall (SYS_brk, end_data_segment);
#ifdef REDIRECT_MEMORY
   }
#endif
}
