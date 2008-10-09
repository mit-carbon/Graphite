#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "transport.h"
#include "pin.H"

class SyscallMdl
{
   private:
      void marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

   public:
      SyscallMdl();
      void runEnter(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);


};

#endif
