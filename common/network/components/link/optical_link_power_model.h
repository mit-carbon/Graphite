#pragma once

#include "link_power_model.h"

class OpticalLinkPowerModel : public LinkPowerModel
{
public:
   OpticalLinkPowerModel(UInt32 num_receivers_per_wavelength, float link_frequency,
                         double waveguide_length, UInt32 link_width);
   ~OpticalLinkPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits);
   
   // Energy parameters specific to OpticalLink
   volatile double getLaserPower()              { return _total_laser_power; }
   volatile double getRingTuningPower()         { return _total_ring_tuning_power; }
   volatile double getDynamicEnergySender()     { return _total_dynamic_energy_sender; }
   volatile double getDynamicEnergyReceiver()   { return _total_dynamic_energy_receiver; }

private:
   UInt32 _num_receivers_per_wavelength;
   
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
   volatile double _total_dynamic_energy_sender;
   volatile double _total_dynamic_energy_receiver;
   
   // Aggregate Parameters
   volatile double _total_ring_tuning_power;
   volatile double _total_laser_power;

};

