#include "pin.H"

#include "lock.h"

class PinLock : public Lock
{
   public:
      PinLock();
      ~PinLock();

      void acquire();
      void release();

   private:
      PIN_LOCK _lock;
};

// -- create -- //
//
// Creates and returns a PinLock.

Lock* Lock::create()
{
   return new PinLock();
}

// -- PinLock -- //

PinLock::PinLock()
{
   InitLock(&_lock);
}


PinLock::~PinLock()
{
}

void PinLock::acquire()
{
   GetLock(&_lock, 1);
}

void PinLock::release()
{
   ReleaseLock(&_lock);
}
