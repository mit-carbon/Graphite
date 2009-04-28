#include "handle_syscalls.h"
#include "fixed_types.h"
#include "syscall_model.h"
#include "simulator.h"
#include "core.h"
#include "core_manager.h"
#include <syscall.h>
#include "redirect_memory.h"
#include "vm_manager.h"

// ----------------------------
// Here to handle rt_sigaction syscall
#include <signal.h>

// ----------------------------
// Here to handle nanosleep syscall
#include <time.h>

// ----------------------------
// FIXME
// Here to examine clone syscall
#include <sched.h>
#include <asm/ldt.h>

// FIXME
// Here to check up on the poll syscall
#include <poll.h>
// Here to check up on the mmap syscall
#include <sys/mman.h>

// FIXME
// -----------------------------------
// Clone stuff

extern PIN_LOCK clone_lock;

// End Clone stuff
// -----------------------------------

void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   ADDRINT syscall_number = PIN_GetSyscallNumber (ctx, syscall_standard);

   string core_null = core ? "CORE != NULL" : "CORE == NULL";
   LOG_PRINT ("syscall_number %d, %s", syscall_number, core_null.c_str());
   
   if (core)
   {
      // Save the syscall number
      core->getSyscallMdl()->saveSyscallNumber (syscall_number);
      
      if ((syscall_number == SYS_write) ||
            (syscall_number == SYS_ioctl) ||
            (syscall_number == SYS_fstat) ||
            (syscall_number == SYS_mprotect) ||
            (syscall_number == SYS_munmap) ||
            (syscall_number == SYS_brk))
      {
         SyscallMdl::syscall_args_t args = syscallArgs (ctx, syscall_standard);
         UInt8 new_syscall = core->getSyscallMdl ()->runEnter (syscall_number, args);
         PIN_SetSyscallNumber (ctx, syscall_standard, new_syscall);
      }
      
      else if ((syscall_number == SYS_mmap) ||
         (syscall_number == SYS_mmap2))
      {
         struct mmap_arg_struct mmap_arg_buf;
         struct mmap_arg_struct *mmap_args_ptr = (struct mmap_arg_struct*) PIN_GetContextReg (ctx, REG_GBX);
         core->accessMemory (Core::NONE, READ, (IntPtr) mmap_args_ptr, (char*) &mmap_arg_buf, sizeof (struct mmap_arg_struct));

         SyscallMdl::syscall_args_t args;
         args.arg0 = (int) &mmap_arg_buf;
         UInt8 new_syscall = core->getSyscallMdl()->runEnter(syscall_number, args);
         PIN_SetSyscallNumber (ctx, syscall_standard, new_syscall);
      }

      else if (syscall_number == SYS_clone)
      {
         // Should never see clone system call execute in a core since 
         // the thread spawner is not a core, and the main thread spawns
         // the thread spawner before it becomes a core
         assert (false);
      }

      else if (syscall_number == SYS_rt_sigprocmask)
      {
         modifyRtsigprocmaskContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_rt_sigsuspend)
      {
         modifyRtsigsuspendContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_rt_sigaction)
      {
         modifyRtsigactionContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_nanosleep)
      {
         modifyNanosleepContext (ctx, syscall_standard);
      }

      else if ((syscall_number == SYS_exit) ||
            (syscall_number == SYS_kill) ||
            (syscall_number == SYS_sigreturn) ||
            (syscall_number == SYS_exit_group))
      {
         // Let the syscall fall through
      }
      
     
      else
      {
         LOG_PRINT ("Unhandled syscall %d", syscall_number);
         assert (false);
      }
   }
}

void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   string core_null = core ? "CORE != NULL" : "CORE == NULL";
   
   if (core)
   {
      ADDRINT syscall_number = core->getSyscallMdl()->retrieveSyscallNumber ();
      
      if (syscall_number == SYS_rt_sigprocmask)
      {
         restoreRtsigprocmaskContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_rt_sigsuspend)
      {
         restoreRtsigsuspendContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_rt_sigaction)
      {
         restoreRtsigactionContext (ctx, syscall_standard);
      }
      
      else if (syscall_number == SYS_nanosleep)
      {
         restoreNanosleepContext (ctx, syscall_standard);
      }
      
      // FIXME
      // Examining clone system call
      else if (syscall_number == SYS_clone)
      {
         // Should never have a core execute a clone system call
         // in the current scheme
         assert (false);
      }
      
      else if ((syscall_number == SYS_fstat) ||
            (syscall_number == SYS_ioctl) ||
            (syscall_number == SYS_write) ||
            (syscall_number == SYS_mprotect))
      {
         UInt8 old_return_val = PIN_GetSyscallReturn (ctx, syscall_standard);
         ADDRINT syscall_return = (ADDRINT) core->getSyscallMdl()->runExit (old_return_val);
         PIN_SetContextReg (ctx, REG_GAX, syscall_return);
      }

      else if ((syscall_number == SYS_mmap2) ||
            (syscall_number == SYS_munmap) ||
            (syscall_number == SYS_brk))
      {
         // TODO:
         // More memory manager magic
         assert (false);
      }
   }
}

void contextChange (THREADID threadIndex, CONTEXT_CHANGE_REASON context_change_reason, const CONTEXT *from, CONTEXT *to, INT32 info, VOID *v)
{
   if (context_change_reason == CONTEXT_CHANGE_REASON_SIGNAL)
   {
      ADDRINT esp_to = PIN_GetContextReg (to, REG_STACK_PTR);
      ADDRINT esp_from = PIN_GetContextReg (from, REG_STACK_PTR);
     
      // Copy over things that the kernel wrote on the stack to
      // the simulated stack
      if (esp_to != esp_from)
      {
         Core *core = Sim()->getCoreManager()->getCurrentCore();
         if (core)
         {
            core->accessMemory (Core::NONE, WRITE, esp_to, (char*) esp_to, esp_from - esp_to + 1);
         }
      }
   }
   
   else if (context_change_reason == CONTEXT_CHANGE_REASON_SIGRETURN)
   {
   }

   else if (context_change_reason == CONTEXT_CHANGE_REASON_FATALSIGNAL)
   {
      assert (false);
   }

   return;
}

void modifyRtsigprocmaskContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args = syscallArgs (ctxt, syscall_standard);
      core->getSyscallMdl()->saveSyscallArgs (args);

      sigset_t *set = (sigset_t*) args.arg1;
      sigset_t *oset = (sigset_t*) args.arg2;

      if (set)
      {
         sigset_t *set_arg = (sigset_t*) core->getSyscallMdl()->copyArgToBuffer (1, (IntPtr) set, sizeof (sigset_t));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) set_arg);
      }

      if (oset)
      {
         sigset_t *oset_arg = (sigset_t*) core->getSyscallMdl()->copyArgToBuffer (2, (IntPtr) oset, sizeof (sigset_t));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) oset_arg);
      }

   }
}

void restoreRtsigprocmaskContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args;
      core->getSyscallMdl()->retrieveSyscallArgs (args);
      sigset_t *set = (sigset_t*) args.arg1;
      sigset_t *oset = (sigset_t*) args.arg2;
      if (oset)
      {
         core->getSyscallMdl()->copyArgFromBuffer (2, (IntPtr) oset, sizeof (sigset_t));
      }
      PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) set);
      PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) oset);
   }
}

void modifyRtsigsuspendContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args = syscallArgs (ctxt, syscall_standard);
      core->getSyscallMdl()->saveSyscallArgs (args);

      sigset_t *unewset = (sigset_t*) args.arg0;
      if (unewset)
      {
         sigset_t *unewset_arg = (sigset_t*) core->getSyscallMdl()->copyArgToBuffer (0, (IntPtr) unewset, sizeof (sigset_t));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 0, (ADDRINT) unewset_arg);
      }
   }
}

void restoreRtsigsuspendContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args;
      core->getSyscallMdl()->retrieveSyscallArgs (args);

      sigset_t *unewset = (sigset_t*) args.arg0;
      PIN_SetSyscallArgument (ctxt, syscall_standard, 0, (ADDRINT) unewset);
   }
}

void modifyRtsigactionContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args = syscallArgs (ctxt, syscall_standard);
      core->getSyscallMdl()->saveSyscallArgs (args);
      
      struct sigaction *act = (struct sigaction*) args.arg1;
      struct sigaction *oact = (struct sigaction*) args.arg2;

      if (act)
      {
         struct sigaction *act_arg = (struct sigaction*) core->getSyscallMdl()->copyArgToBuffer (1, (IntPtr) act, sizeof (struct sigaction));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) act_arg);
      }

      if (oact)
      {
         struct sigaction *oact_arg = (struct sigaction*) core->getSyscallMdl()->copyArgToBuffer (2, (IntPtr) oact, sizeof (struct sigaction));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) oact_arg);
      }
   }
}

void restoreRtsigactionContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args;
      core->getSyscallMdl()->retrieveSyscallArgs (args);

      struct sigaction *act = (struct sigaction*) args.arg1;
      struct sigaction *oact = (struct sigaction*) args.arg2;

      if (oact)
      {
         core->getSyscallMdl()->copyArgFromBuffer (2, (IntPtr) oact, sizeof (struct sigaction));
      }

      PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) act);
      PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) oact);
   }
}

void modifyNanosleepContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore ();
   if (core)
   {
      SyscallMdl::syscall_args_t args = syscallArgs (ctxt, syscall_standard);
      core->getSyscallMdl ()->saveSyscallArgs (args);

      struct timespec *req = (struct timespec*) args.arg0;
      struct timespec *rem = (struct timespec*) args.arg1;

      if (req)
      {
         struct timespec *req_arg = (struct timespec*) core->getSyscallMdl ()->copyArgToBuffer (0, (IntPtr) req, sizeof (struct timespec));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 0, (ADDRINT) req_arg);
      }

      if (rem)
      {
         struct timespec *rem_arg = (struct timespec*) core->getSyscallMdl ()->copyArgToBuffer (1, (IntPtr) rem, sizeof (struct timespec));
         PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) rem_arg);
      }
   }
}

void restoreNanosleepContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   Core *core = Sim()->getCoreManager()->getCurrentCore();
   if (core)
   {
      SyscallMdl::syscall_args_t args;
      core->getSyscallMdl()->retrieveSyscallArgs (args);

      struct timespec *req = (struct timespec*) args.arg0;
      struct timespec *rem = (struct timespec*) args.arg1;

      if (rem)
      {
         core->getSyscallMdl()->copyArgFromBuffer (1, (IntPtr) rem, sizeof (struct timespec));
      }

      PIN_SetSyscallArgument (ctxt, syscall_standard, 0, (ADDRINT) req);
      PIN_SetSyscallArgument (ctxt, syscall_standard, 1, (ADDRINT) rem);
   }
}

SyscallMdl::syscall_args_t syscallArgs (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
{
   SyscallMdl::syscall_args_t args;
   args.arg0 = PIN_GetSyscallArgument (ctxt, syscall_standard, 0);
   args.arg1 = PIN_GetSyscallArgument (ctxt, syscall_standard, 1);
   args.arg2 = PIN_GetSyscallArgument (ctxt, syscall_standard, 2);
   args.arg3 = PIN_GetSyscallArgument (ctxt, syscall_standard, 3);
   args.arg4 = PIN_GetSyscallArgument (ctxt, syscall_standard, 4);
   args.arg5 = PIN_GetSyscallArgument (ctxt, syscall_standard, 5);

   return args;
}

// ---------------------------------------------------------------
// Clone stuff
// ---------------------------------------------------------------

// void modifyCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
// {
//    Core *core = Sim()->getCoreManager()->getCurrentCore();
//    if (core)
//    {
//       // GetLock (&clone_lock, 1);
//       SyscallMdl::syscall_args_t args = syscallArgs (ctxt, syscall_standard);
//       core->getSyscallMdl()->saveSyscallArgs (args);
// 
//       int *parent_tidptr = (int*) PIN_GetSyscallArgument (ctxt, syscall_standard, 2);
//       struct user_desc *newtls = (struct user_desc*) PIN_GetSyscallArgument (ctxt, syscall_standard, 3);
//       int *child_tidptr = (int*) PIN_GetSyscallArgument (ctxt, syscall_standard, 4);
// 
//       if (parent_tidptr)
//       {
//          // FIXME
//          // The behavior of copyArgToBuffer will have to change for multi-machine simulations
//          // We can't just copy contents of the memory location pointed to by the argument by default
//          // since the memory may not be valid on this machine
//          int *parent_tidptr_arg = (int*) core->getSyscallMdl()->copyArgToBuffer (2, (IntPtr) parent_tidptr, sizeof (int));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) parent_tidptr_arg);
//       }
// 
//       if (newtls)
//       {
//          struct user_desc *newtls_arg = (struct user_desc*) core->getSyscallMdl()->copyArgToBuffer (3, (IntPtr) newtls, sizeof (struct user_desc));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 3, (ADDRINT) newtls_arg);
//       }
// 
//       if (child_tidptr)
//       {
//          int *child_tidptr_arg = (int*) core->getSyscallMdl()->copyArgToBuffer (4, (IntPtr) child_tidptr, sizeof (int));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 4, (ADDRINT) child_tidptr_arg);
//       }
//    }
// }
// 
// void restoreCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard)
// {
//    Core *core = Sim()->getCoreManager()->getCurrentCore();
//    if (core)
//    {
//       SyscallMdl::syscall_args_t args;
//       core->getSyscallMdl()->retrieveSyscallArgs(args);
//       int *parent_tidptr = (int*) args.arg2;
//       struct user_desc *newtls = (struct user_desc*) args.arg3;
//       int *child_tidptr = (int*) args.arg4;
// 
//       if (parent_tidptr)
//       {
//          core->getSyscallMdl()->copyArgFromBuffer(2, (IntPtr) parent_tidptr, sizeof (int));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 2, (ADDRINT) parent_tidptr);
//       }
// 
//       if (newtls)
//       {
//          core->getSyscallMdl()->copyArgFromBuffer(3, (IntPtr) newtls, sizeof (struct user_desc));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 3, (ADDRINT) newtls);
//       }
// 
//       if (child_tidptr)
//       {
//          core->getSyscallMdl()->copyArgFromBuffer (4, (IntPtr) child_tidptr, sizeof (int));
//          PIN_SetSyscallArgument (ctxt, syscall_standard, 4, (ADDRINT) child_tidptr);
//       }
// 
//       // ReleaseLock (&clone_lock);
//    }
// }
// 
