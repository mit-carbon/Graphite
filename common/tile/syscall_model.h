#ifndef SYSCALL_MODEL_H
#define SYSCALL_MODEL_H

#include <iostream>

#include "packetize.h"
#include "network.h"
#include "fixed_types.h"
#include "core.h"

class SyscallMdl
{
   public:
      struct syscall_args_t
      {
          IntPtr arg0;
          IntPtr arg1;
          IntPtr arg2;
          IntPtr arg3;
          IntPtr arg4;
          IntPtr arg5;
      };

      SyscallMdl(Core *core);

      IntPtr runExit(IntPtr old_return);
      IntPtr runEnter(IntPtr syscall_number, syscall_args_t &args);
      
      // ------------------------------------------------------
      // New stuff added with Memory redirection

      void saveSyscallArgs (syscall_args_t &args);
      void retrieveSyscallArgs (syscall_args_t &args);
      void saveSyscallNumber (IntPtr syscall_number);
      IntPtr retrieveSyscallNumber();

      void *copyArgToBuffer(UInt32 arg_num, IntPtr arg_addr, UInt32 size);
      void copyArgFromBuffer(UInt32 arg_num, IntPtr arg_addr, UInt32 size);

      //---------------------------------------------------------

   private:
      
      // ------------------------------------------------------
      // New stuff added with Memory redirection
      
      static const UInt32 m_scratchpad_size = 512;
      static const UInt32 m_num_syscall_args = 6;
     
      IntPtr m_syscall_number;
      bool m_called_enter;
      IntPtr m_ret_val;
      struct syscall_args_t m_saved_args;
      char m_scratchpad [m_num_syscall_args] [m_scratchpad_size];
      
      // ------------------------------------------------------

      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      Network *m_network;

      IntPtr marshallOpenCall(syscall_args_t &args);
      IntPtr marshallReadCall(syscall_args_t &args);
      IntPtr marshallWriteCall(syscall_args_t &args);
      IntPtr marshallWritevCall(syscall_args_t &args);
      IntPtr marshallCloseCall(syscall_args_t &args);
      IntPtr marshallLseekCall(syscall_args_t &args);
      IntPtr marshallAccessCall(syscall_args_t &args);
      IntPtr marshallStatCall(syscall_args_t &args);
      IntPtr marshallFstatCall(syscall_args_t &args);
      IntPtr marshallIoctlCall(syscall_args_t &args);
      IntPtr marshallGetpidCall(syscall_args_t &args);
      IntPtr marshallReadaheadCall(syscall_args_t &args);
      IntPtr marshallPipeCall(syscall_args_t &args);
      IntPtr marshallMmapCall(syscall_args_t &args);
      IntPtr marshallMunmapCall(syscall_args_t &args);
      IntPtr marshallBrkCall(syscall_args_t &args);
      IntPtr marshallFutexCall(syscall_args_t &args);

      IntPtr marshallUnlinkCall(syscall_args_t &args);
      IntPtr marshallRmdirCall(syscall_args_t &args);

      IntPtr handleClockGettimeCall(syscall_args_t &args);
      IntPtr marshallGetCwdCall(syscall_args_t &args);
      IntPtr marshallSchedSetAffinityCall(syscall_args_t &args);
      IntPtr marshallSchedGetAffinityCall(syscall_args_t &args);

      // Helper functions
      UInt32 getStrLen (char *str);

      struct mmap_arg_struct
      {
         unsigned long addr;
         unsigned long len;
         unsigned long prot;
         unsigned long flags;
         unsigned long fd;
         unsigned long offset;
      };
};

#endif
