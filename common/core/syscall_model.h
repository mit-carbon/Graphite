#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "packetize.h"
#include "transport.h"
#include "network.h"
#include "pin.H"
#include "mcp.h" //has the enum for mcp message types

class SyscallMdl
{
   private:
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

      void runEnter(int commid, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(int commid, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
};

#endif
