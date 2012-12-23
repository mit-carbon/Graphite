#pragma once

#include "fixed_types.h"

class ShmemPerfModel
{
public:
   ShmemPerfModel();
   ~ShmemPerfModel();

   void setCycleCount(UInt64 count);
   UInt64 getCycleCount();
   void incrCycleCount(UInt64 count);
   void updateCycleCount(UInt64 count);
   
   void enable()     { _enabled = true;  }
   void disable()    { _enabled = false; }

private:
   UInt64 _cycle_count;
   bool _enabled;
};
