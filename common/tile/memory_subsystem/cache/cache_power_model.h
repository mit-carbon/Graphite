#pragma once

#include <string>
#include "fixed_types.h"

class CachePowerModel
{
   public:
      CachePowerModel(std::string type, UInt32 size, UInt32 blocksize,
            UInt32 associativity, UInt32 delay, volatile float frequency);
      ~CachePowerModel() {}

      void updateDynamicEnergy() { _total_dynamic_energy += _dynamic_energy; }
      volatile double getTotalDynamicEnergy() { return _total_dynamic_energy; }
      volatile double getTotalStaticPower() { return _total_static_power; }

      void outputSummary(std::ostream& out);
      static void dummyOutputSummary(std::ostream& out);

   private:
      volatile double _total_dynamic_energy;
      volatile double _total_static_power;
      volatile double _dynamic_energy;
};
