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
   public:
      SyscallMdl(Network *net);

      void runEnter(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      void runExit(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);

   private:
      bool m_called_enter;
      int m_ret_val;

      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      Network *m_network;

      int marshallOpenCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallReadCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallWriteCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallCloseCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
      int marshallAccessCall(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
};

#endif
