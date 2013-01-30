#include "cond.h"

#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <limits.h>
#include "log.h"

ConditionVariable::ConditionVariable()
   : m_futx(0)
{
}

ConditionVariable::~ConditionVariable()
{
}

void ConditionVariable::wait(Lock& lock)
{
   m_lock.acquire();

   // Wait
   m_futx = 0;

   m_lock.release();

   lock.release();

   syscall(SYS_futex, (void*) &m_futx, FUTEX_WAIT, 0, NULL, NULL, 0);

   lock.acquire();
}

void ConditionVariable::signal()
{
   m_lock.acquire();

   m_futx = 1;

   syscall(SYS_futex, (void*) &m_futx, FUTEX_WAKE, 1, NULL, NULL, 0);

   m_lock.release();
}

void ConditionVariable::broadcast()
{
   m_lock.acquire();

   m_futx = 1;

   syscall(SYS_futex, (void*) &m_futx, FUTEX_WAKE, INT_MAX, NULL, NULL, 0);

   m_lock.release();
}
