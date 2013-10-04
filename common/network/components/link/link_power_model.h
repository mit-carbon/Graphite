#pragma once

#include "fixed_types.h"
#include "time_types.h"

class LinkPowerModel
{
public:
   LinkPowerModel();
   virtual ~LinkPowerModel();

   virtual void setDVFS(double frequency, double voltage, const Time& curr_time) = 0;
   virtual void computeEnergy(const Time& curr_time) = 0;
   double getDynamicEnergy()  { return _total_dynamic_energy;  }
   double getStaticEnergy()   { return _total_static_energy;   }
   
protected:
   // Output parameters 
   double _total_dynamic_energy;
   double _total_static_energy;
   // Last energy compute time
   Time _last_energy_compute_time;
};
