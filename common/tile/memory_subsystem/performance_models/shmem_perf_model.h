#pragma once

#include "fixed_types.h"
#include "time_types.h"

class ShmemPerfModel
{
public:
   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCurrTime(Time time);
   Time getCurrTime();
   void incrCurrTime(Time time);
   void updateCurrTime(Time time);

   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }
   
   bool isEnabled(){return _enabled;}

private:
   Time _curr_time;
   bool _enabled;
};
