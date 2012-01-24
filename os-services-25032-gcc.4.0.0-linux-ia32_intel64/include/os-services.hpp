#ifndef OS_SERVICES_H
#define OS_SERVICES_H

#include <stddef.h>


namespace OS_SERVICES
{
    class ITHREAD;
    class ITHREAD_RUNNER;

    // A collection of thread services.
    //
    class ITHREADS
    {
    public:
        static ITHREADS *GetSingleton();
    
        // Spawn a new low-level thread.  The "stack_size" parameter tells the number of
        // bytes of stack space to allocate.  The new thread starts running at "runner->RunThread()".
        //
        virtual ITHREAD *Spawn(size_t stack_size, ITHREAD_RUNNER *runner) = 0;

        virtual void YieldMe() = 0;
        virtual unsigned long GetSysId() = 0;
        virtual void Exit() = 0;
        virtual ~ITHREADS() {}
    };

    class ITHREAD
    {
    public:
        virtual void Exit() = 0;
        virtual bool IsActive() = 0;
        virtual ~ITHREAD() {}
    };

    class ITHREAD_RUNNER
    {
    public:
        virtual void RunThread(ITHREAD *self) = 0;
        virtual ~ITHREAD_RUNNER() {}
    };
}
#endif
