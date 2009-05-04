#ifndef __HANDLE_SYSCALLS_H__
#define __HANDLE_SYSCALLS_H__

#include "pin.H"
#include "syscall_model.h"

void syscallEnterRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
void syscallExitRunModel(CONTEXT *ctx, SYSCALL_STANDARD syscall_standard);
void contextChange (THREADID threadIndex, CONTEXT_CHANGE_REASON context_change_reason, const CONTEXT *from, CONTEXT *to, INT32 info, VOID *v);
void threadStart (THREADID threadIndex, CONTEXT *ctx, INT32 flags, VOID *v);

SyscallMdl::syscall_args_t syscallArgs (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyRtsigprocmaskContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreRtsigprocmaskContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);

void modifyCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyRtsigsuspendContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreRtsigsuspendContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyRtsigactionContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreRtsigactionContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyNanosleepContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreNanosleepContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);

void modifyUnameContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreUnameContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyUgetrlimitContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreUgetrlimitContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifySet_thread_areaContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreSet_thread_areaContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void modifyCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);
void restoreCloneContext (CONTEXT *ctxt, SYSCALL_STANDARD syscall_standard);

#endif /* __HANDLE_SYSCALLS_H__ */
