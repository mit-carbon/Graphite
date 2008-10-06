#include "syscall_model.h"
#include "sys/syscall.h"


SyscallMdl::SyscallMdl()
{
}

void SyscallMdl::run(ADDRINT syscall_number, ADDRINT syscall_arg_0, ADDRINT syscall_arg_1,
               ADDRINT syscall_arg_2, ADDRINT syscall_arg_3, ADDRINT syscall_arg_4, ADDRINT syscall_arg_5)
{
   switch((int)syscall_number)
   {
      case SYS_open:
         cout << "Syscall number: " << (int)syscall_number << endl;
         break;
      default:
         break;
   }
}

