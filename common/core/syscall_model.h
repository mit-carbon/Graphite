#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>

#include "message_types.h"
#include "packetize.h"
#include "transport.h"
#include "network.h"
#include "pin.H"

class SyscallMdl
{
   private:
      bool called_enter;
      int ret_val;

      UnstructuredBuffer send_buff;
      UnstructuredBuffer recv_buff;
      Network *the_network;

      int marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallReadCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallWriteCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallCloseCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallAccessCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

   public:
      SyscallMdl(Network *net);

      void runEnter(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
};

#endif
