#pragma once

#include "electrical_network_link_model.h"
#include "fixed_types.h"

class ElectricalNetworkRepeatedLinkModel : public ElectricalNetworkLinkModel
{
public:
   ElectricalNetworkRepeatedLinkModel(NetworkModel* model, volatile float link_frequency,
                                      volatile double link_length, UInt32 link_width);
   ~ElectricalNetworkRepeatedLinkModel();

   void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1);

private:
   // Delay Parameters
   volatile double _delay_per_mm;

   // Static Power Parameters
   volatile double _static_link_power_per_mm;

   // Dynamic Power Parameters
   volatile double _dynamic_link_energy_per_mm;
};
