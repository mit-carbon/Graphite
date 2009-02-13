#ifndef NET_THREAD_RUNNER_H
#define NET_THREAD_RUNNER_H

#include "os-services.hpp"
#include "mcp.h"

class NetThreadRunner : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
       NetThreadRunner();
       virtual void RunThread(OS_SERVICES::ITHREAD *me);
};

#endif // NET_THREAD_RUNNER_H
