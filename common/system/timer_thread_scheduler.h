#ifndef TIMER_THREAD_SCHEDULER_SERVER_H
#define TIMER_THREAD_SCHEDULER_SERVER_H

#include "thread_scheduler.h"

class ThreadManager;
class TileManager;

class TimerThreadScheduler : public ThreadScheduler
{
public:
   TimerThreadScheduler(ThreadManager *thread_manager, TileManager *tile_manager);
   ~TimerThreadScheduler();

   virtual void yieldThread();
   virtual void masterYieldThread(ThreadYieldRequest* req);

private:

   std::vector< std::vector<UInt32> > m_last_start_time;
   UInt32 m_thread_switch_time;
};

#endif // TIMER_THREAD_SCHEDULER_SERVER_H
