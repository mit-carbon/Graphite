
#include "os-services.hpp"
#include "mcp.h"

class NetThreadRunner : public OS_SERVICES::ITHREAD_RUNNER
{
   public:
       NetThreadRunner();
       virtual void RunThread(OS_SERVICES::ITHREAD *me);

       static void StopNetThreads();

   private:
       static bool net_threads_continue;
};

