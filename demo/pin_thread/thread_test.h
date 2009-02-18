#ifndef THREAD_TEST_H
#define THREAD_TEST_H

#include "os-services.hpp"

/*
ITHREAD *my_thread_p;
ITHREAD_RUNNER my_thread_runner;
my_thread_p = ITHREADS::GetSingleton()->Spawn(4096, &my_thread_runner);
my_thread_runner.RunThread(my_thread_p);
*/

class TestThread : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
      virtual void RunThread(OS_SERVICES::ITHREAD *me);
};

#endif
