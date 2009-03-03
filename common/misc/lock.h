#ifndef LOCK_H
#define LOCK_H

#include <pthread.h>

class Lock
{
public:
   Lock();
   ~Lock();

   void acquire();
   void release();

private:
   pthread_mutex_t _mutx;
};

class ScopedLock
{
   Lock &_lock;

public:
   ScopedLock(Lock &lock)
      : _lock(lock)
   {
      _lock.acquire();
   }

   ~ScopedLock()
   {
      _lock.release();
   }
};

#endif // LOCK_H
