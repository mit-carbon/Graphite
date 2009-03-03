#ifndef PIN_THREAD_H
#define PIN_THREAD_H

#include "thread.h"
#include "os-services.hpp"

class PinThreadRunner : public OS_SERVICES::ITHREAD_RUNNER
{
public:
   PinThreadRunner(Thread::ThreadFunc func, void *param);
   void RunThread(OS_SERVICES::ITHREAD *me);

private:
   Thread::ThreadFunc m_func;
   void *m_param;
};

class PinThread : public Thread
{
public:
   PinThread(ThreadFunc func, void *param);
   ~PinThread();
   void run();

private:
   static const int STACK_SIZE=65536;

   PinThreadRunner *m_thread_runner;
   OS_SERVICES::ITHREAD *m_thread_p;
};

#endif // PIN_THREAD_H
