#include "cond.h"

ConditionVariable::ConditionVariable()
{
   pthread_cond_init(&_cond, NULL);
}

ConditionVariable::~ConditionVariable()
{
   pthread_cond_destroy(&_cond);
}

void ConditionVariable::wait(Lock& _lock)
{
   // FIXME: We can only use pthread locks here
   // _lock should be of type 'pthread_mutex_t'
   pthread_cond_wait(&_cond, &_lock.getMutx());
}

void ConditionVariable::signal()
{
   pthread_cond_signal(&_cond);
}

void ConditionVariable::broadcast()
{
   pthread_cond_broadcast(&_cond); 
}
