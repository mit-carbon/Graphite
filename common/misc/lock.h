#pragma once

#include <pthread.h>

class Lock
{
public:
   Lock();
   ~Lock();

   void acquire();
   void release();
   bool tryLock();

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
