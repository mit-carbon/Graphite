#ifndef COND_H
#define COND_H

class Lock;

// Our condition variable interface is slightly different from
// pthreads in that the mutex associated with the condition variable
// is built into the condition variable itself.

class ConditionVariable
{
public:
   ConditionVariable();
   ~ConditionVariable();

   void acquire();
   void release();

   // must acquire lock before entering wait. will own lock upon exit.
   void wait();
   void signal();
   void broadcast();
   
private:
   int _numWaiting;
   int _futx;
   Lock *_lock;

};

#endif // COND_H
