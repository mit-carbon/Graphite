#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "pin.H"

class SyscallMdl
{
   public:
      SyscallMdl();
      void runEnter(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
   private:
};

#endif
