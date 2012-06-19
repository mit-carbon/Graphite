#ifndef __SEMAPHORE_H__
#define __SEMAPHORE_H__

#include "lock.h"

class Semaphore
{
public:
   Semaphore(int count = 0);
   ~Semaphore();

   void wait();
   void signal();
   void broadcast();

private:
   int _count;
   int _numWaiting;
   int _futx;
   Lock _lock;
};

#endif



