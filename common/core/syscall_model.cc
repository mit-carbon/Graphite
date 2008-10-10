#include "syscall_model.h"
#include "sys/syscall.h"
#include "chip.h"
#include "transport.h"

bool called_open = false;
bool called_read = false;
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

   if(called_open)
   {
      PIN_SetContextReg(ctx, REG_EAX, ret_val);
      called_open = false;
   }
   if(called_read)
   {
      

   }
}

void SyscallMdl::runEnter(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   // FIXME: need to use PT_get_from_server_reply_buff and PT_put_to_server_buff
   // for marshalling data around

   int syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);

   switch(syscall_number)
   {
      case SYS_open:
      {
         char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         if(!strcmp(path,"./common/tests/file_io/input"))
         {
            called_open = true;
            cout << "open(" << path << ")" << endl;

            marshallOpenCall(ctx, syscall_standard);

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
         if ( fd == 8 )
	 {
	    called_read = true;
            cout << "read(" << fd << hex << ", " << read_buf << dec << ", " << read_count << ")" << endl;

            marshallReadCall(ctx, syscall_standard);


            // safer than letting the original syscall go
            // PIN_SetSyscallNumber(ctx, syscall_standard, SYS_getpid);
	 }

	 break;
      }

      // case SYS_write:
      // {
      //    cout << "write(";
      //    cout << (int)PIN_GetSyscallArgument(ctx, syscall_standard, 0) << ", \"";
      //    cout << (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 1) << "\")" << endl;
      //    break;
      // }
      // case SYS_close:
      // {
      //    cout << "close(" << (int)PIN_GetSyscallArgument(ctx, syscall_standard, 0) << ")" << endl;
      //    break;
      // }
      // case SYS_exit:
      //    cout << "exit()" << endl;
      //    break;
      case -1:
         break;
      default:
//         cout << "SysCall: " << (int)syscall_number << endl;
         break;
   }

}

void SyscallMdl::marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   /*
       Transmit

       Field               Type
       -----------------|--------
       MSGTYPE_SYSCALL     int
       COMM_ID             int
       SYSCALL_NUMBER      char
       FILE_NAME           char[]
       STATUS_FLAGS        int

       Receive
       
       Field               Type
       -----------------|--------
       STATUS              int       

   */

   int commid;
   char buf[1024];

   int syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);
   char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   int flags = (int)PIN_GetSyscallArgument(ctx, syscall_standard, 1);


   commRank(&commid);
   int *buf_int = (int *) buf;
   buf_int[0] = 0;
   buf_int[1] = commid;
   buf[sizeof(int)*2] = syscall_number;

   cout << "sending syscall number: " << (int)syscall_number << endl;

   UInt32 offset = sizeof(int)*2+1;
   UInt32 size = strlen(path);
   memcpy(&buf[offset], path, size);
   offset += size;

   // null terminate
   buf[offset] = '\0';
   offset++;

   cout << "flags expected: " << flags << endl;

   memcpy(&buf[offset], &flags, sizeof(flags));
   offset += sizeof(flags);

   // offset is now the total length
   the_network->getTransport()->ptSendToMCP((UInt8 *) buf, offset);

   UInt32 len;
   char *reply = (char *) the_network->getTransport()->ptRecvFromMCP(&len);
   assert( len == sizeof(int) );

   int *reply_int = (int *) reply;
   ret_val = reply_int[0];

   cout << "got retval: " << ret_val << endl;

}


void SyscallMdl::marshallReadCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{

   /*
       Transmit

       Field               Type
       -----------------|--------
       MSGTYPE_SYSCALL     int
       COMM_ID             int
       SYSCALL_NUMBER      char
       FILE_DESCRIPTOR     int
       COUNT               size_t

       Receive
       
       Field               Type
       -----------------|--------
       STATUS              int
       BUFFER              void *       

   */

   //int syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);
   




}
