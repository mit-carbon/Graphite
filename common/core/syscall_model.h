#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "pin.H"

class SyscallMdl
{
   public:
      SyscallMdl();
      void run(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
   private:
};

#endif
