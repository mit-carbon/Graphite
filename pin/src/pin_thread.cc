#include "pin_thread.h"
#include <assert.h>

PinThreadRunner::PinThreadRunner(Thread::ThreadFunc func, void *param)
   : m_func(func)
   , m_param(param)
{
}

void PinThreadRunner::RunThread(OS_SERVICES::ITHREAD *)
{
   m_func(m_param);
}

PinThread::PinThread(ThreadFunc func, void *param)
   : m_thread_runner(new PinThreadRunner(func, param))
   , m_thread_p(NULL)
{
}

PinThread::~PinThread()
{
   delete m_thread_p;
   delete m_thread_runner;
}

void PinThread::run()
{
   m_thread_p = OS_SERVICES::ITHREADS::GetSingleton()->Spawn(4096, m_thread_runner);
   assert(m_thread_p);
}

Thread* Thread::create(ThreadFunc func, void *param)
{
   return new PinThread(func, param);
}
