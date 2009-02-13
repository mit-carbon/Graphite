#ifndef SIM_THREAD_RUNNER_H
#define SIM_THREAD_RUNNER_H

#include "os-services.hpp"
#include "mcp.h"

class SimThreadRunner : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
      SimThreadRunner();
      virtual void RunThread(OS_SERVICES::ITHREAD *me);
};

#endif // SIM_THREAD_RUNNER_H
