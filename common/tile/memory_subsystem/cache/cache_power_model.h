#pragma once

#include <string>
#include "cache_info.h"
#include "fixed_types.h"

class CachePowerModel
{
public:
   CachePowerModel(std::string type, UInt32 size, UInt32 blocksize,
         UInt32 associativity, UInt32 delay, float frequency);
   ~CachePowerModel() {}

   void updateDynamicEnergy(UInt32 op);
   double getTotalDynamicEnergy() { return _total_dynamic_energy; }
   double getTotalStaticPower() { return _total_static_power; }

   void outputSummary(std::ostream& out);
   static void dummyOutputSummary(std::ostream& out);

private:
   CachePower _cache_power;
   double _total_dynamic_energy;
   double _total_static_power;
};
