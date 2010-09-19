#pragma once

#include <pthread.h>

#include "pin.H"
#include "carbon_user.h"
#include "fixed_types.h"

namespace lite
{

void routineCallback(RTN rtn, void* v);

carbon_thread_t emuCarbonSpawnThread(CONTEXT* context, thread_func_t thread_func, void* arg);
int emuPthreadCreate(CONTEXT* context, pthread_t* thread_ptr, pthread_attr_t* attr, thread_func_t thread_func, void* arg);
void emuCarbonJoinThread(CONTEXT* context, carbon_thread_t tid);
int emuPthreadJoin(CONTEXT* context, pthread_t thread, void** thead_return);
IntPtr nullFunction();

AFUNPTR getFunptr(CONTEXT* context, string func_name);

}
