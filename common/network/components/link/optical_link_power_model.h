#pragma once

class LaserModes;

#include "link_power_model.h"
#include "contrib/dsent/dsent_contrib.h"
#include <string>

class OpticalLinkPowerModel : public LinkPowerModel
{
public:
   OpticalLinkPowerModel(LaserModes& laser_modes, UInt32 num_receivers_per_wavelength,
                         float link_frequency, double waveguide_length, UInt32 link_width);
   ~OpticalLinkPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints);
   
   // Energy parameters specific to OpticalLink
   volatile double getStaticLeakagePower()      { return _static_power_leakage;     }
   volatile double getStaticLaserPower()        { return _static_power_laser;       }
   volatile double getStaticHeatingPower()      { return _static_power_heating;     }
   
   volatile double getDynamicEnergy()           { return _total_dynamic_energy;     }

private:
   // Mapping between the name of a laser type defined in carbon_sim.cfg to a laser type used by DSENT
   const std::string getDSENTLaserType(const std::string& carbon_laser_type) const;
   // Mapping between the name of a ring tuning strategy defined in carbon_sim.cfg to a tuning strategy
   // used by DSENT
   const std::string getDSENTTuningStrategy(const std::string& carbon_tuning_strategy) const;
   
   
private:
   // DSENT model for the datapath link
   dsent_contrib::DSENTOpticalLink* _dsent_link;
   // DSENT model for the selector link
   dsent_contrib::DSENTOpticalLink* _dsent_sel;

   // Possible laser modes - (idle, unicast, broadcast)
   LaserModes _laser_modes;

   // Has a select network
   bool _sel_link;
   // Number of readers
   unsigned int _num_receivers_per_wavelength;
   
   // Aggregate Parameters
   volatile double _static_power_leakage;
   volatile double _static_power_laser;
   volatile double _static_power_heating;

   volatile double _total_dynamic_energy;
  
   void initializeDynamicEnergyCounters();
};

