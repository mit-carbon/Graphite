#ifndef LOCK_H
#define LOCK_H

class Lock
{
 public:

   virtual ~Lock() { }

   virtual void acquire() = 0;
   virtual void release() = 0;

   static Lock* create();
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
