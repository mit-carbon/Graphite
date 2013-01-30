#pragma once

#include <string>
using std::string;

#include "optical_link_model.h"
#include "link_power_model.h"
#include "contrib/dsent/dsent_contrib.h"

class OpticalLinkPowerModel : public LinkPowerModel
{
public:
   OpticalLinkPowerModel(OpticalLinkModel::LaserModes laser_modes, OpticalLinkModel::LaserType laser_type,
                         string ring_tuning_strategy, UInt32 num_readers_per_wavelength,
                         float link_frequency, double waveguide_length, UInt32 link_width);
   ~OpticalLinkPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints);
   
   // Energy parameters specific to OpticalLink
   volatile double getStaticLeakagePower()      { return _static_power_leakage;     }
   volatile double getStaticLaserPower()        { return _static_power_laser;       }
   volatile double getStaticHeatingPower()      { return _static_power_heating;     }

private:
   // Mapping between the name of a laser type defined in carbon_sim.cfg to a laser type used by DSENT
   const string getDSENTLaserType(const OpticalLinkModel::LaserType& laser_type) const;
   // Mapping between the name of a ring tuning strategy defined in carbon_sim.cfg to a tuning strategy
   // used by DSENT
   const string getDSENTTuningStrategy(const string& carbon_tuning_strategy) const;
   
private:
   // DSENT model for the datapath link
   dsent_contrib::DSENTOpticalLink* _dsent_data_link;
   // DSENT model for the selector link
   dsent_contrib::DSENTOpticalLink* _dsent_select_link;
   // Has a select network
   bool _select_link_enabled;
   // Number of readers
   unsigned int _num_readers_per_wavelength;
   
   // Aggregate Parameters
   volatile double _static_power_leakage;
   volatile double _static_power_laser;
   volatile double _static_power_heating;
};
