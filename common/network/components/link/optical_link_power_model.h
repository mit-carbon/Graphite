#pragma once

#include <map>
#include <string>
using std::map;
using std::string;

#include "optical_link_model.h"
#include "link_power_model.h"
#include "contrib/dsent/dsent_contrib.h"

class OpticalLinkPowerModel : public LinkPowerModel
{
public:
   OpticalLinkPowerModel(OpticalLinkModel::LaserModes laser_modes, OpticalLinkModel::LaserType laser_type,
                         string ring_tuning_strategy, UInt32 num_readers_per_wavelength,
                         double frequency, double voltage, double waveguide_length, UInt32 link_width);
   ~OpticalLinkPowerModel();

   void setDVFS(double frequency, double voltage, const Time& curr_time);
   void computeEnergy(const Time& curr_time);
   void updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints);
   
private:
   // DSENT model for the datapath link
   map<double, dsent_contrib::DSENTOpticalLink*> _dsent_data_link_map;
   dsent_contrib::DSENTOpticalLink* _dsent_data_link;
   // DSENT model for the selector link
   map<double, dsent_contrib::DSENTOpticalLink*> _dsent_select_link_map;
   dsent_contrib::DSENTOpticalLink* _dsent_select_link;
   
   // Has a select network
   bool _select_link_enabled;
   // Number of readers
   unsigned int _num_readers_per_wavelength;
   
   // Mapping between the name of a laser type defined in carbon_sim.cfg to a laser type used by DSENT
   const string getDSENTLaserType(const OpticalLinkModel::LaserType& laser_type) const;
   // Mapping between the name of a ring tuning strategy defined in carbon_sim.cfg to a tuning strategy
   // used by DSENT
   const string getDSENTTuningStrategy(const string& carbon_tuning_strategy) const;

   // Static power
   double getStaticPower() const;
};
