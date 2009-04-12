#ifndef COND_H
#define COND_H

#include <pthread.h>
#include "lock.h"

// Our condition variable interface is slightly different from
// pthreads in that the mutex associated with the condition variable
// is built into the condition variable itself.

class ConditionVariable
{
   public:
      ConditionVariable();
      ~ConditionVariable();

      // must acquire lock before entering wait. will own lock upon exit.
      void wait(Lock& _lock);
      void signal();
      void broadcast();

   private:
      int m_futx;
      Lock m_lock;
};

#endif // COND_H
