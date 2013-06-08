#include "pin_thread.h"
#include <assert.h>

PinThread::PinThread(ThreadFunc func, void *param)
   : m_thread_p((THREADID) NULL)
   , m_func(func)
   , m_param(param)
 {
 }

PinThread::~PinThread()
{
}

void PinThread::run()
{
   m_thread_p = PIN_SpawnInternalThread(m_func, m_param, STACK_SIZE, NULL);
   assert(m_thread_p != INVALID_THREADID);
}

Thread* Thread::create(ThreadFunc func, void *param)
{
   return new PinThread(func, param);
}
