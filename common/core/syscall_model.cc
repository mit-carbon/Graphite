#include "syscall_model.h"
#include "sys/syscall.h"
#include "chip.h"
#include "transport.h"

bool called_enter = false;
int ret_val = 0;

SyscallMdl::SyscallMdl(Network *net)
   :the_network(net)
{
}

void SyscallMdl::runExit(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   //if only the code below worked in enter...
   //int return_addr = PIN_GetContextReg(ctx, REG_INST_PTR);
   //return_addr += 2;
   //PIN_SetContextReg(ctx, REG_INST_PTR, return_addr);
   //PIN_ExecuteAt(ctx);

   if(called_enter)
   {
      PIN_SetContextReg(ctx, REG_EAX, ret_val);
      called_enter = false;
   }
}

void SyscallMdl::runEnter(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   // Reset the buffers for the new transmission
   recv_buff.clear(); 
   send_buff.clear(); 
   
   // FIXME: this should be taken from an enum not hardcoded to 0
   int msg_type = 0;
   int commid;
   commRank(&commid);
   UInt8 syscall_number = (UInt8) PIN_GetSyscallNumber(ctx, syscall_standard);
   
   send_buff << msg_type << commid << syscall_number;   

   switch(syscall_number)
   {
      case SYS_open:
      {
         char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         if(!strcmp(path,"./common/tests/file_io/input"))
         {
            called_enter = true;
            cerr << "open(" << path << ")" << endl;

            ret_val = marshallOpenCall(ctx, syscall_standard);

            // safer than letting the original syscall go
            PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
         }

         break;
      }
      case SYS_read:
      {
	 int fd = PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         void *read_buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
         size_t read_count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);
         if ( fd == 0xcc )
	 {
	    called_enter = true;
            cerr << "read(" << fd << hex << ", " << read_buf << dec << ", " << read_count << ")" << endl;

            ret_val = marshallReadCall(ctx, syscall_standard);

            // safer than letting the original syscall go
            PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
	 }

	 break;
      }

      case SYS_write:
      {
	 int fd = PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         void *write_buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
         size_t write_count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);
         if ( fd == 0xcc )
	 {
	    called_enter = true;
            cerr << "write(" << fd << hex << ", " << write_buf << dec << ", " << write_count << ")" << endl;

            ret_val = marshallWriteCall(ctx, syscall_standard);

            // safer than letting the original syscall go
            PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
	 }         

	 break;
      }
      case SYS_close:
      {
	 int fd = PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         if ( fd == 0xcc )
	 {
	    called_enter = true;
            cerr << "close(" << fd << ")" << endl;

            ret_val = marshallCloseCall(ctx, syscall_standard);

            // safer than letting the original syscall go
            PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
	 }

	 break;
      }
      // case SYS_exit:
      //    cerr << "exit()" << endl;
      //    break;
      case -1:
         break;
      default:
//         cerr << "SysCall: " << (int)syscall_number << endl;
         break;
   }

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

   char *path = (char *) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   int flags = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   UInt32 len_fname = strlen(path) + 1;

   send_buff << len_fname << make_pair(path, len_fname) << flags;
   the_network->getTransport()->ptSendToMCP((UInt8 *) send_buff.getBuffer(), send_buff.size());

   UInt32 length = 0;
   UInt8 *res_buff = the_network->getTransport()->ptRecvFromMCP(&length);
   assert( length == sizeof(int) );
   recv_buff << make_pair(res_buff, length);

   int status;
   recv_buff >> status;

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

   cerr << "Entering syscall model marshall read" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   void *buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   size_t count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);
      
   send_buff << fd << count;
   the_network->getTransport()->ptSendToMCP((UInt8 *) send_buff.getBuffer(), send_buff.size());   
   
   cerr << "sent to mcp " << send_buff.size() << " bytes" << endl;

   UInt32 length = 0;
   UInt8 *res_buff = the_network->getTransport()->ptRecvFromMCP(&length);
   cerr << "received from mcp" << endl;

   assert( length >= sizeof(int) );
   recv_buff << make_pair(res_buff, length);

   int bytes;
   recv_buff >> bytes;

   if ( bytes != -1 )
   {
      recv_buff >> make_pair(buf, bytes);
   } 
   else 
   {
      assert( recv_buff.size() == 0 );
   }
   cerr << "Exiting syscall model marshall read" << endl;

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

   cerr << "Entering syscall model marshall write" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   void *buf = (void *) PIN_GetSyscallArgument(ctx, syscall_standard, 1);
   size_t count = (size_t) PIN_GetSyscallArgument(ctx, syscall_standard, 2);
      
   send_buff << fd << count << make_pair(buf, count);
   the_network->getTransport()->ptSendToMCP((UInt8 *) send_buff.getBuffer(), send_buff.size());      

   UInt32 length = 0;
   UInt8 *res_buff = the_network->getTransport()->ptRecvFromMCP(&length);
   assert( length == sizeof(int) );
   recv_buff << make_pair(res_buff, length);

   int status;
   recv_buff >> status;

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

   cerr << "Entering syscall model marshall close" << endl;

   int fd = (int) PIN_GetSyscallArgument(ctx, syscall_standard, 0);      
   send_buff << fd;
   the_network->getTransport()->ptSendToMCP((UInt8 *) send_buff.getBuffer(), send_buff.size());      

   UInt32 length = 0;
   UInt8 *res_buff = the_network->getTransport()->ptRecvFromMCP(&length);
   assert( length == sizeof(int) );
   recv_buff << make_pair(res_buff, length);

   int status;
   recv_buff >> status;

   return status;
}
