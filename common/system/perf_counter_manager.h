#ifndef __PERF_COUNTER_MANAGER_H__
#define __PERF_COUNTER_MANAGER_H__

#include "thread_manager.h"

class PerfCounterManager
{
   private:
      ThreadManager *m_thread_manager;

   public:
      PerfCounterManager(ThreadManager *thread_manager);
      ~PerfCounterManager();
};

#endif /* __PERF_COUNTER_MANAGER_H__ */
