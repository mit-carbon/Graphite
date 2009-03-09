#include "syscall_model.h"
#include "sys/syscall.h"
#include "transport.h"
#include "config.h"

using namespace std;

SyscallMdl::SyscallMdl(Network *net)
      : m_called_enter(false),
      m_ret_val(0),
      m_network(net)
{
}

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
      m_called_enter = true;
      m_ret_val = marshallAccessCall(args);
      break;
   case SYS_brk:
      //uncomment the following when our shared-mem handles mallocs properly
      //m_called_enter = true;
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

   // If we are simulating shared memory, then we simply put
   // the address in the message. Otherwise, we need to put
   // the data in the message as well.

   // FIXME: This is disabled until memory redirection is
   // functional. We should also ask ourselves if this is the behavior
   // we want (message passing vs shared memory).
   if (false && Config::getSingleton()->isSimulatingSharedMemory())
      m_send_buff << fd << count << (int)buf;
   else
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


