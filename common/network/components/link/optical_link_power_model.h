#pragma once

class LaserModes;

#include "link_power_model.h"

class OpticalLinkPowerModel : public LinkPowerModel
{
public:
   OpticalLinkPowerModel(LaserModes& laser_modes, UInt32 num_receivers_per_wavelength,
                         float link_frequency, double waveguide_length, UInt32 link_width);
   ~OpticalLinkPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints);
   
   // Energy parameters specific to OpticalLink
   volatile double getStaticLaserPower()        { return _total_static_laser_power;       }
   volatile double getStaticRingTuningPower()   { return _total_static_ring_tuning_power; }
   volatile double getStaticPowerTx()           { return _total_static_power_tx;          }
   volatile double getStaticPowerRx()           { return _total_static_power_rx;          }

   volatile double getDynamicLaserEnergy()      { return _total_dynamic_laser_energy;     }
   volatile double getDynamicEnergyTx()         { return _total_dynamic_energy_tx;        }
   volatile double getDynamicEnergyRx()         { return _total_dynamic_energy_rx;        }

private:
   // Possible laser modes - (idle, unicast, broadcast)
   LaserModes _laser_modes;

   // Num receivers to broadcast on a wavelenth
   UInt32 _num_receivers_per_wavelength;
   
   // Unit parameters
   volatile double _ring_tuning_power;
   volatile double _laser_power_per_receiver;
   volatile double _electrical_tx_static_power;
   volatile double _electrical_rx_static_power;
   volatile double _electrical_tx_dynamic_energy;
   volatile double _electrical_rx_dynamic_energy;
   
   // Aggregate Parameters
   volatile double _total_static_laser_power;
   volatile double _total_static_ring_tuning_power;
   volatile double _total_static_power_tx;
   volatile double _total_static_power_rx;
   volatile double _total_dynamic_laser_energy;
   volatile double _total_dynamic_energy_tx;
   volatile double _total_dynamic_energy_rx;
  
   void initializeDynamicEnergyCounters();
};

