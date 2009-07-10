#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>

#include "message_types.h"
#include "packetize.h"
#include "transport.h"
#include "network.h"
#include "fixed_types.h"

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
      
      // ------------------------------------------------------
      // New stuff added with Memory redirection

      void saveSyscallArgs (syscall_args_t &args);
      void retrieveSyscallArgs (syscall_args_t &args);
      void saveSyscallNumber (carbon_reg_t syscall_number);
      carbon_reg_t retrieveSyscallNumber ();

      void *copyArgToBuffer (unsigned int arg_num, IntPtr arg_addr, unsigned int size);
      void copyArgFromBuffer (unsigned int arg_num, IntPtr arg_addr, unsigned int size);

      //---------------------------------------------------------

   private:
      
      // ------------------------------------------------------
      // New stuff added with Memory redirection
      
      static const unsigned int m_scratchpad_size = 512;
      static const unsigned int m_num_syscall_args = 6;
     
      carbon_reg_t m_syscall_number;
      bool m_called_enter;
      int m_ret_val;
      struct syscall_args_t m_saved_args;
      char m_scratchpad [m_num_syscall_args] [m_scratchpad_size];
      
      // ------------------------------------------------------

      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      Network *m_network;

      carbon_reg_t marshallOpenCall(syscall_args_t &args);
      carbon_reg_t marshallReadCall(syscall_args_t &args);
      carbon_reg_t marshallWriteCall(syscall_args_t &args);
      carbon_reg_t marshallCloseCall(syscall_args_t &args);
      carbon_reg_t marshallAccessCall(syscall_args_t &args);
      carbon_reg_t marshallGetpidCall(syscall_args_t &args);
      carbon_reg_t marshallReadaheadCall(syscall_args_t &args);
      carbon_reg_t marshallPipeCall(syscall_args_t &args);
      carbon_reg_t marshallFstatCall(syscall_args_t &args);
      carbon_reg_t marshallIoctlCall(syscall_args_t &args);
      carbon_reg_t marshallMprotectCall(syscall_args_t &args);
      carbon_reg_t marshallMmapCall(syscall_args_t &args);
      carbon_reg_t marshallMmap2Call(syscall_args_t &args);
      carbon_reg_t marshallMunmapCall(syscall_args_t &args);
      carbon_reg_t marshallBrkCall(syscall_args_t &args);
      carbon_reg_t marshallFutexCall(syscall_args_t &args);

};

#endif
