#include "syscall_model.h"
#include "sys/syscall.h"


SyscallMdl::SyscallMdl()
{
}

void SyscallMdl::run(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   // No need to make this call remote
   // if(rank == 0)
   //    return; 

//   int syscall_number = 0;
#if 0
   int syscall_number = PIN_GetSyscallNumber(ctx, syscall_standard);

   switch(syscall_number)
   {
      case SYS_open:
      {
         char *path = (char *)PIN_GetSyscallArgument(ctx, syscall_standard, 0);
         cout << "open(" << path << ")" << endl;
         //open((char *)syscall_arg_0);

         //normally we would send this call to the server
         //and it would return a valid FD on that machine
         //for now we are hard-coding it to be 8 so that
         //we can test other parts of the code
         //TODO: send this call to the syscall server
         //int fd = 8;


         break;
      }
      case SYS_exit:
         cout << "exit()" << endl;
         break;
      case -1:
         break;
      default:
         cout << "SysCall: " << (int)syscall_number << endl;
         break;
   }
#endif
}

