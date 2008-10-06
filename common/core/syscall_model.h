#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "pin.H"

class SyscallMdl
{
   public:
      SyscallMdl();
      void run(ADDRINT syscall_number, ADDRINT syscall_arg_0, ADDRINT syscall_arg_1,
            ADDRINT syscall_arg_2, ADDRINT syscall_arg_3, ADDRINT syscall_arg_4, ADDRINT syscall_arg_5);
   private:
};

#endif
