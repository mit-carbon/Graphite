#pragma once

#include "network_link_model.h"
#include "fixed_types.h"

class OpticalNetworkLinkModel : public NetworkLinkModel
{
public:
   OpticalNetworkLinkModel(volatile float link_frequency, volatile double waveguide_length, UInt32 link_width);
   ~OpticalNetworkLinkModel();

   UInt64 getDelay();
   volatile double getStaticPower();
   void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1);
   volatile double getDynamicEnergy() { return _total_dynamic_energy; }

   void resetCounters() { _total_dynamic_energy = 0; }

private:
   volatile float _frequency;
   volatile double _waveguide_length;
   UInt32 _link_width;
   UInt32 _num_receivers_per_wavelength;

   // Delay parameters
   volatile double _waveguide_delay_per_mm;
   UInt64 _e_o_conversion_delay;
   UInt64 _o_e_conversion_delay;

   UInt64 _net_optical_link_delay;

   // Static Power parameters
   volatile double _ring_tuning_power;
   volatile double _laser_power;
   volatile double _electrical_tx_static_power;
   volatile double _electrical_rx_static_power;
   volatile double _clock_static_power_tx;
   volatile double _clock_static_power_rx;

   // Dynamic Power parameters
   volatile double _electrical_tx_dynamic_energy;
   volatile double _electrical_rx_dynamic_energy;
   volatile double _total_dynamic_energy;
};
