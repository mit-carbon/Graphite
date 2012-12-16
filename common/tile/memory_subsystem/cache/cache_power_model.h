#pragma once

#include <string>
#include "cache_info.h"
#include "fixed_types.h"

class CachePowerModel
{
public:
   CachePowerModel(std::string type, UInt32 size, UInt32 blocksize,
         UInt32 associativity, UInt32 delay, volatile float frequency);
   ~CachePowerModel() {}

   void updateDynamicEnergy(UInt32 op);
   volatile double getTotalDynamicEnergy() { return _total_dynamic_energy; }
   volatile double getTotalStaticPower() { return _total_static_power; }

   void outputSummary(std::ostream& out);
   static void dummyOutputSummary(std::ostream& out);

private:
   CachePower _cache_power;
   volatile double _total_dynamic_energy;
   volatile double _total_static_power;
};
