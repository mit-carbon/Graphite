#pragma once

#include "fixed_types.h"
#include "time_types.h"

class ShmemPerfModel
{
public:
   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCurrTime(Time& time);
   Time getCurrTime();
   void incrCurrTime(Latency& time);
   void updateCurrTime(Time& time);

   void setCycleCount(UInt64 count);
   UInt64 getCycleCount();
   void incrCycleCount(UInt64 count);
   void updateCycleCount(UInt64 count);

   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }

private:
   Time _curr_time;
   //UInt64 _cycle_count;
   bool _enabled;
};
