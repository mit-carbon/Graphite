#pragma once

#include "pin.H"
#include "fixed_types.h"

namespace lite
{

void handleFutexSyscall(CONTEXT* ctx);
void syscallEnterRunModel(THREADID threadIndex, CONTEXT* ctx, SYSCALL_STANDARD syscall_standard, void* v);
void syscallExitRunModel(THREADID threadIndex, CONTEXT* ctx, SYSCALL_STANDARD syscall_standard, void* v);

}
