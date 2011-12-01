#include <cassert>
#include "statistics_thread.h"
#include "log.h"

StatisticsThread::StatisticsThread(StatisticsManager* manager)
   : _thread(NULL)
   , _statistics_manager(manager)
   , _finished(false)
   , _time(0)
   , _flag(false)
{}

StatisticsThread::~StatisticsThread()
{
   delete _thread;
}

void
StatisticsThread::run()
{
   LOG_PRINT("Statistics thread starting...");

   while (!_finished)
   {
      _lock.acquire();
      _cond_var.wait(_lock);
      _lock.release();
      
      if (_time == UINT64_MAX_)
      {
         // Simulation over
         _finished = true;
      }
      else // Simulation still running
      {
         assert(_flag);
         // Call statistics manager
         _statistics_manager->outputPeriodicSummary();
         _flag = false;
      }
   }

   LOG_PRINT("Statistics thread exiting");
}

void
StatisticsThread::start()
{
   _thread = Thread::create(this);
   _thread->run();
}

void
StatisticsThread::finish()
{
   _time = UINT64_MAX_;
   _cond_var.signal();

   // Wait till the thread exits
   while (!_finished)
      sched_yield();
}

void
StatisticsThread::notify(UInt64 time)
{
   if ((time % _statistics_manager->getSamplingInterval()) == 0)
   {
      LOG_ASSERT_WARNING(!_flag, "Sampling interval too small");
      _flag = true;

      _time = time;
      _cond_var.signal();
   }
}
