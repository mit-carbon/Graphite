#pragma once

#include "statistics_manager.h"
#include "fixed_types.h"
#include "thread.h"
#include "cond.h"

class StatisticsThread : public Runnable
{
public:
   StatisticsThread(StatisticsManager* manager);
   ~StatisticsThread();

   void start();
   void finish();
   void notify(UInt64 time);

private:
   void run();

   Thread* _thread;
   StatisticsManager* _statistics_manager;
   bool _finished;
   UInt64 _time;
   ConditionVariable _cond_var;
   Lock _lock;    // Kind of useless. used just because of the interface of condition variable operations
   bool _flag;
};
