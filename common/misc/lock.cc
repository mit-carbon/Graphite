#include "lock.h"

Lock::Lock()
{
   pthread_mutex_init(&_mutx, NULL);
}

Lock::~Lock()
{
   pthread_mutex_destroy(&_mutx);
}

void Lock::acquire()
{
   pthread_mutex_lock(&_mutx);
}

void Lock::release()
{
   pthread_mutex_unlock(&_mutx);
}

bool Lock::tryLock()
{
   int res = pthread_mutex_trylock(&_mutx);
   return (res == 0);
}
