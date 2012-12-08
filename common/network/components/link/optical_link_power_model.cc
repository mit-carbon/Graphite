#include <cmath>

#include "optical_link_power_model.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "log.h"

using namespace dsent_contrib;
using std::string;

OpticalLinkPowerModel::OpticalLinkPowerModel(OpticalLinkModel::LaserModes laser_modes, OpticalLinkModel::LaserType laser_type,
                                             string ring_tuning_strategy, UInt32 num_readers_per_wavelength,
                                             float link_frequency, double waveguide_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, waveguide_length, link_width)
   , _num_readers_per_wavelength(num_readers_per_wavelength)
{
   // Tuning strategy and laser type read from config file
   // Map to dsent tuning strategy
   string dsent_laser_type = getDSENTLaserType(laser_type);
   string dsent_tuning_strategy = getDSENTTuningStrategy(ring_tuning_strategy);
  
   // The maximum number of simultaneous readers on an SWMR link
   UInt32 max_simultaneous_readers = 0;
   if (laser_modes.broadcast)
      max_simultaneous_readers = _num_readers_per_wavelength;
   else if (laser_modes.unicast)
      max_simultaneous_readers = 1;
  
   LOG_PRINT("DSENT Optical data link. Frequency: %f GHz, Waveguide length: %g mm, "
             "Num readers: %i, Max simultaneous readers: %i, "
             "Link width: %u, Tuning strategy: %s, Laser type: %s",
             link_frequency, waveguide_length, _num_readers_per_wavelength, max_simultaneous_readers,
             link_width, dsent_tuning_strategy.c_str(), dsent_laser_type.c_str());
   // Create a link
   _dsent_data_link = new DSENTOpticalLink(
            link_frequency,                 // Core data rate (Right now, no serdes is assumed)
            link_frequency,                 // Link data rate (Right now, no serdes is assumed)
            waveguide_length / 1e3,         // Link length, convert to meters (m)
            _num_readers_per_wavelength,    // Number of readers on the wavelength
            max_simultaneous_readers,       // Maximum number of simultaneous readers
            link_width,                     // Core flit width
            dsent_tuning_strategy,          // Ring tuning strategy
            dsent_laser_type,               // Laser type
            DSENTInterface::getSingleton()
        );
   LOG_PRINT("DSENT Optical data link. Instantiated");
   
   // Create separate select network if necessary for the given configuration
   unsigned int select_link_width = (laser_modes.unicast) ? ceilLog2(_num_readers_per_wavelength) : 0;
   if (select_link_width > 0)
   {
      LOG_PRINT("DSENT Optical select link. Frequency: %f GHz, Waveguide length: %g mm, "
                "Num readers: %i, Max simultaneous readers: %i, "
                "Link width: %u, Tuning strategy: %s, Laser type: %s",
                link_frequency, waveguide_length, _num_readers_per_wavelength, _num_readers_per_wavelength,
                select_link_width, dsent_tuning_strategy.c_str(), dsent_laser_type.c_str());
      
      _dsent_select_link = new DSENTOpticalLink(
                link_frequency,                 // Core data rate (Right now, no serdes is assumed)
                link_frequency,                 // Link data rate (Right now, no serdes is assumed)
                waveguide_length / 1e3,         // Link length, convert to meters (m)
                _num_readers_per_wavelength,    // Number of readers
                _num_readers_per_wavelength,    // Maximum number of simultaneous readers
                select_link_width,              // Core flit width
                dsent_tuning_strategy,          // Ring tuning strategy
                dsent_laser_type,               // Laser type
                DSENTInterface::getSingleton()
            );
   
      LOG_PRINT("DSENT Optical select link. Instantiated");
      _select_link_enabled = true;
   }
   else
   {
      _select_link_enabled = false;
   }
        
   // Static Power
   _static_power_leakage = _dsent_data_link->get_static_power_leakage();
   _static_power_laser = _dsent_data_link->get_static_power_laser();
   _static_power_heating = _dsent_data_link->get_static_power_heating();   
   if (_select_link_enabled)
   {
      _static_power_leakage += _dsent_select_link->get_static_power_leakage();
      _static_power_laser += _dsent_select_link->get_static_power_laser();
      _static_power_heating += _dsent_select_link->get_static_power_heating();
   }
   _total_static_power = _static_power_leakage + _static_power_laser + _static_power_heating;
}

OpticalLinkPowerModel::~OpticalLinkPowerModel()
{
    delete _dsent_data_link;
    if (_select_link_enabled)
       delete _dsent_select_link;
}

void
OpticalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints)
{    
   // Add dynamic energies
   _total_dynamic_energy += _dsent_data_link->calc_dynamic_energy(num_flits, num_endpoints);
   // Select network needed during unicasts/broadcasts
   if (_select_link_enabled)
      _total_dynamic_energy += _dsent_select_link->calc_dynamic_energy(num_flits, _num_readers_per_wavelength);
}

const string
OpticalLinkPowerModel::getDSENTTuningStrategy(const string& ring_tuning_strategy) const
{
   if (ring_tuning_strategy == "athermal") return "AthermalWithTrim";
   else if (ring_tuning_strategy == "full_thermal") return "FullThermal";
   else if (ring_tuning_strategy == "thermal_reshuffle") return "ThermalWithBitReshuffle";
   else if (ring_tuning_strategy == "electrical_assist") return "ElectricalAssistWithBitReshuffle";

   LOG_PRINT_ERROR("Unknown ring tuning strategy.");     
   return "";
}

const string
OpticalLinkPowerModel::getDSENTLaserType(const OpticalLinkModel::LaserType& laser_type) const
{
   if (laser_type == OpticalLinkModel::STANDARD)
      return "Standard";
   else if (laser_type == OpticalLinkModel::THROTTLED)
      return "Throttled";
   else
   {
      LOG_PRINT_ERROR("Unknown laser type.");
      return "";
   }
}
