#include "lock.h"

#include "log.h"

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
