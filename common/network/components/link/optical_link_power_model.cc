#include <cmath>

#include "optical_link_power_model.h"
#include "dvfs_manager.h"
#include "utils.h"
#include "log.h"

using namespace dsent_contrib;

OpticalLinkPowerModel::OpticalLinkPowerModel(OpticalLinkModel::LaserModes laser_modes, OpticalLinkModel::LaserType laser_type,
                                             string ring_tuning_strategy, UInt32 num_readers_per_wavelength,
                                             double frequency, double voltage, double waveguide_length, UInt32 link_width)
   : LinkPowerModel()
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
 
   const DVFSManager::DVFSLevels& dvfs_levels = DVFSManager::getDVFSLevels();
   for (DVFSManager::DVFSLevels::const_iterator it = dvfs_levels.begin(); it != dvfs_levels.end(); it++)
   {
      double current_voltage = (*it).first;
      double current_frequency = (*it).second;

      LOG_PRINT("DSENT Optical data link. Frequency: %f GHz, Waveguide length: %g mm, "
                "Num readers: %i, Max simultaneous readers: %i, "
                "Link width: %u, Tuning strategy: %s, Laser type: %s",
                current_frequency, waveguide_length, _num_readers_per_wavelength, max_simultaneous_readers,
                link_width, dsent_tuning_strategy.c_str(), dsent_laser_type.c_str());
      // Create a link
      _dsent_data_link_map[current_voltage] = new DSENTOpticalLink(
               current_frequency * 1e9,        // Core data rate, convert to Hz (Right now, no serdes is assumed)
               current_frequency * 1e9,        // Link data rate, convert to Hz (Right now, no serdes is assumed)
               current_voltage,                // Current voltage
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
                   current_frequency, waveguide_length, _num_readers_per_wavelength, _num_readers_per_wavelength,
                   select_link_width, dsent_tuning_strategy.c_str(), dsent_laser_type.c_str());
         
         _dsent_select_link_map[current_voltage] = new DSENTOpticalLink(
                   current_frequency * 1e9,        // Core data rate, convert to Hz (Right now, no serdes is assumed)
                   current_frequency * 1e9,        // Link data rate, convert to Hz (Right now, no serdes is assumed)
                   current_voltage,                // Current voltage
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
   }

   // Set current data/select link model
   _dsent_data_link = _dsent_data_link_map[voltage];
   _dsent_select_link = _dsent_select_link_map[voltage];
}

OpticalLinkPowerModel::~OpticalLinkPowerModel()
{
    delete _dsent_data_link;
    if (_select_link_enabled)
       delete _dsent_select_link;
}

void
OpticalLinkPowerModel::setDVFS(double frequency, double voltage, const Time& curr_time)
{
   // Compute dynamic/static energy upto this point
   computeEnergy(curr_time);

   // Set appropriate data/select link models
   _dsent_data_link = _dsent_data_link_map[voltage];
   LOG_ASSERT_ERROR(_dsent_data_link, "Could not find DSENT Optical Data Link model for voltage(%g)", voltage);
   _dsent_select_link = _dsent_select_link_map[voltage];
   LOG_ASSERT_ERROR(_dsent_select_link, "Could not find DSENT Optical Select Link model for voltage(%g)", voltage);
}

void
OpticalLinkPowerModel::computeEnergy(const Time& curr_time)
{
   // Compute the interval between current time and time when energy was last computed
   double time_interval = (curr_time - _last_energy_compute_time).toSec();
   // Increment total static energy
   _total_static_energy += getStaticPower() * time_interval;
   // Set _last_energy_compute_time to curr_time
   _last_energy_compute_time = curr_time;
}

double
OpticalLinkPowerModel::getStaticPower() const
{
   // Static Power
   double static_power = _dsent_data_link->get_static_power_leakage() + 
                         _dsent_data_link->get_static_power_laser() +
                         _dsent_data_link->get_static_power_heating();   
   if (_select_link_enabled)
   {
      static_power += (_dsent_select_link->get_static_power_leakage() + 
                       _dsent_select_link->get_static_power_laser() +
                       _dsent_select_link->get_static_power_heating());
   }
   return static_power;
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
