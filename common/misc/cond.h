#ifndef COND_H
#define COND_H

#include "lock.h"

class ConditionVariable
{
public:
   ConditionVariable();
   ~ConditionVariable();

   void wait();
   void signal();
   void broadcast();
   
private:
   int _numWaiting;
   int _futx;
   Lock *_lock;

};

#endif // COND_H
