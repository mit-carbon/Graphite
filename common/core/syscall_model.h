#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>

#include "message_types.h"
#include "packetize.h"
#include "transport.h"
#include "network.h"

class SyscallMdl
{
   public:
      struct syscall_args_t
      {
          int arg0;
          int arg1;
          int arg2;
          int arg3;
          int arg4;
          int arg5;
      };

      SyscallMdl(Network *net);

      carbon_reg_t runExit(int old_return);
      UInt8 runEnter(UInt8 syscall_number, syscall_args_t &args);

   private:

      bool m_called_enter;
      int m_ret_val;

      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      Network *m_network;

      carbon_reg_t marshallOpenCall(syscall_args_t &args);
      carbon_reg_t marshallReadCall(syscall_args_t &args);
      carbon_reg_t marshallWriteCall(syscall_args_t &args);
      carbon_reg_t marshallCloseCall(syscall_args_t &args);
      carbon_reg_t marshallAccessCall(syscall_args_t &args);

};

#endif
