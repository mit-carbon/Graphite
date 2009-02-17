#include "syscall_model.h"
#include "sys/syscall.h"
#include "transport.h"
#include "config.h"

SyscallMdl::SyscallMdl(Network *net)
      : m_called_enter(false),
      m_ret_val(0),
      m_network(net)
{
}

void SyscallMdl::runExit(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   //if only the code below worked in enter...
   //int return_addr = PIN_GetContextReg(ctx, REG_INST_PTR);
   //return_addr += 2;
   //PIN_SetContextReg(ctx, REG_INST_PTR, return_addr);
   //PIN_ExecuteAt(ctx);

   if (m_called_enter)
   {
#ifdef TARGET_IA32E
      PIN_SetContextReg(ctx, REG_RAX, m_ret_val);
#else
      PIN_SetContextReg(ctx, REG_EAX, m_ret_val);
#endif
      m_called_enter = false;
   }
}

void SyscallMdl::runEnter(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   // Reset the buffers for the new transmission
   m_recv_buff.clear();
   m_send_buff.clear();

   int msg_type = MCP_MESSAGE_SYS_CALL;

   UInt8 syscall_number = (UInt8) PIN_GetSyscallNumber(ctx, syscall_standard);

   m_send_buff << msg_type << syscall_number;

   switch (syscall_number)
   {
   case SYS_open:
   {
      m_called_enter = true;
      m_ret_val = marshallOpenCall(ctx, syscall_standard);
      break;
   }
   case SYS_read:
   {
      m_called_enter = true;
      m_ret_val = marshallReadCall(ctx, syscall_standard);
      break;
   }

   case SYS_write:
   {
      m_called_enter = true;
      m_ret_val = marshallWriteCall(ctx, syscall_standard);
      break;
   }
   case SYS_close:
   {
      m_called_enter = true;
      m_ret_val = marshallCloseCall(ctx, syscall_standard);
      break;
   }
   case SYS_access:
      m_called_enter = true;
      m_ret_val = marshallAccessCall(ctx, syscall_standard);
      break;
   case SYS_brk:
      //uncomment the following when our shared-mem handles mallocs properly
      //m_called_enter = true;
      break;

      // case SYS_exit:
      //    cerr << "exit()" << endl;
      //    break;
   case -1:
      break;
   default:
//            cerr << "SysCall: " << (int)syscall_number << endl;
      break;
   }

   if (m_called_enter)
      PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);


}


int SyscallMdl::marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
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

   //cerr << "Entering SyscallMdl::marshallOpen()" << endl;

   char *path = (char *) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   int flags = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   UInt32 len_fname = strlen(path) + 1;

   // cerr << "open(" << path << ")" << endl;

   m_send_buff << len_fname << make_pair(path, len_fname) << flags;
   m_network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*)recv_pkt.data;

   return status;
}


int SyscallMdl::marshallReadCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
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

   //cerr << "Entering syscall model marshall read" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   void *buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   size_t count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);

   // cerr << "read(" << fd << hex << ", " << buf << dec << ", " << count << ")" << endl;

//if shared mem, provide the buf to read into
   m_send_buff << fd << count << (int)buf;
   m_network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   //cerr << "sent to mcp " << m_send_buff.size() << " bytes" << endl;

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);

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
   //cerr << "Exiting syscall model marshall read" << endl;

   delete [](Byte*)recv_pkt.data;

   return bytes;
}


int SyscallMdl::marshallWriteCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
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

   //cerr << "Entering syscall model marshall write" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   void *buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   size_t count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);

   // cerr << "write(" << fd << hex << ", " << buf << dec << ", " << count << ")" << endl;

   // If we are simulating shared memory, then we simply put
   // the address in the message. Otherwise, we need to put
   // the data in the message as well.
   if (g_config->isSimulatingSharedMemory())
      m_send_buff << fd << count << (int)buf;
   else
      m_send_buff << fd << count << make_pair(buf, count);

   m_network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*) recv_pkt.data;

   return status;
}

int SyscallMdl::marshallCloseCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
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

   //cerr << "Entering syscall model marshall close" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);

   // cerr << "close(" << fd  << ")" << endl;

   m_send_buff << fd;
   m_network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);
   assert(recv_pkt.length == sizeof(int));
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   int status;
   m_recv_buff >> status;

   delete [](Byte*) recv_pkt.data;

   return status;
}

int SyscallMdl::marshallAccessCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   //cerr << "Entering SyscallMdl::marshallAccessCall()" << endl;

   char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   int mode = (int)PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   UInt32 len_fname = strlen(path) + 1;

   // pack the data
   m_send_buff << len_fname << make_pair(path, len_fname) << mode;

   // send the data
   m_network->netSend(g_config->getMCPCoreNum(), MCP_REQUEST_TYPE, m_send_buff.getBuffer(), m_send_buff.size());

   // get a result
   NetPacket recv_pkt;
   recv_pkt = m_network->netRecv(g_config->getMCPCoreNum(), MCP_RESPONSE_TYPE);

   // Create a buffer out of the result
   m_recv_buff << make_pair(recv_pkt.data, recv_pkt.length);

   // return the result
   int result;
   m_recv_buff >> result;

   delete [](Byte*) recv_pkt.data;

   return result;
}


