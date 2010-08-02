#pragma once

#include "electrical_network_link_model.h"
#include "fixed_types.h"

class ElectricalNetworkRepeatedLinkModel : public ElectricalNetworkLinkModel
{
public:
   ElectricalNetworkRepeatedLinkModel(volatile float link_frequency, volatile double link_length, UInt32 link_width);
   ~ElectricalNetworkRepeatedLinkModel();

   UInt64 getDelay();
   volatile double getStaticPower();
   void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1);
   volatile double getDynamicEnergy() { return _total_dynamic_energy; }

private:
   volatile float _frequency;
   volatile double _link_length;
   UInt32 _link_width;

   // Delay Parameters
   volatile double _delay_per_mm;
   UInt64 _net_link_delay;

   // Static Power Parameters
   volatile double _static_link_power_per_mm;

   // Dynamic Power Parameters
   volatile double _dynamic_link_energy_per_mm;
   volatile double _total_dynamic_energy;
};
