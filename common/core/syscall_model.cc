#include "syscall_model.h"
#include "sys/syscall.h"
#include "chip.h"
#include "transport.h"

bool called_open = false;
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
   int commid;
   UInt8 buf [1024];

   int syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);
   char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
   int flags = (int)PIN_GetSyscallArgument(ctx, syscall_standard, 1);


   commRank(&commid);
   UInt32 *buf32 = (UInt32 *)buf;
   buf32[0] = 0;
   buf32[1] = commid;
   buf[sizeof(UInt32)*2] = syscall_number;

   cout << "sending syscall number: " << (int)syscall_number << endl;

   UInt32 len = sizeof(UInt32)*2+1;

   memcpy(&buf[len], path, strlen(path));
   len += strlen(path);

   // null terminate
   buf[len] = 0;
   len++;

   cout << "flags expected: " << flags << endl;

   memcpy(&buf[len], &flags, sizeof(flags));
   len += sizeof(flags);

   the_network->getTransport()->ptSendToMCP(buf, len);

   // re-use our old lenth variable now that it is already sent
   UInt8 *reply = the_network->getTransport()->ptRecvFromMCP(&len);

   int *reply_int = (int *)reply;
   ret_val = reply_int[0];

   cout << "got retval: " << ret_val << endl;

}
