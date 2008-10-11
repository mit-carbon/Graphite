#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>
#include "packetize.h"
#include "transport.h"
#include "network.h"
#include "pin.H"

class SyscallMdl
{
   private:
      UnstructuredBuffer send_buff;
      UnstructuredBuffer recv_buff;
      Network *the_network;

      void marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void marshallReadCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

   public:
      SyscallMdl(Network *net);

      void runEnter(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(int rank, CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

};

#endif
