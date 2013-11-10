#pragma once

#include "fixed_types.h"
#include "time_types.h"

class ShmemPerfModel
{
public:
   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCurrTime(const Time& time);
   Time getCurrTime();
   void incrCurrTime(const Time& time);
   void updateCurrTime(const Time& time);

   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }
   
private:
   Time _curr_time;
   bool _enabled;
};
