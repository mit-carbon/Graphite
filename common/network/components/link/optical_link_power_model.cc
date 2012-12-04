#include <cmath>

#include "optical_link_model.h"
#include "optical_link_power_model.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "log.h"

using namespace dsent_contrib;
using std::string;

OpticalLinkPowerModel::OpticalLinkPowerModel(LaserModes& laser_modes, UInt32 num_receivers_per_wavelength,
                                             float link_frequency, double waveguide_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, waveguide_length, link_width)
   , _laser_modes(laser_modes)
   , _num_receivers_per_wavelength(num_receivers_per_wavelength)
{

   // Tuning strategy and laser type read from config file
   string carbon_tuning_strategy;
   string carbon_laser_type;

   try
   {
      // Read ring and laser tuning strategies
      carbon_laser_type = Sim()->getCfg()->getString("link_model/optical/power/ring_tuning_strategy");
      carbon_tuning_strategy = Sim()->getCfg()->getString("link_model/optical/power/laser_type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read laser type and/or tuning strategy from the cfg file. Defaulting to athermal rings and a standard laser");
     
      carbon_tuning_strategy = "athermal";
      carbon_laser_type = "standard";
   }

   // Map to dsent tuning strategy
   string dsent_tuning_strategy = getDSENTTuningStrategy(carbon_tuning_strategy);
   string dsent_laser_type = getDSENTLaserType(carbon_laser_type);
   
   // The maximum number of simultaneous readers on an SWMR link
   unsigned int max_readers = 0;
   if (laser_modes.broadcast) max_readers = num_receivers_per_wavelength;
   else if (laser_modes.unicast) max_readers = 1;
   
   // Create a link
   _dsent_link = new DSENTOpticalLink(
            link_frequency,                 // Core data rate (Right now, no serdes is assumed)
            link_frequency,                 // Link data rate (Right now, no serdes is assumed)
            waveguide_length / 1e3,         // Link length, convert to meters (m)
            num_receivers_per_wavelength,   // Number of readers
            max_readers,                    // Maximum number of simultaneous readers
            link_width,                     // Core flit width
            dsent_tuning_strategy,          // Ring tuning strategy
            dsent_laser_type,               // Laser type
            DSENTInterface::getSingleton()
        );
   
   // Create separate select network if necessary for the given configuration
   unsigned int select_link_width = (laser_modes.unicast) ? ceilLog2(num_receivers_per_wavelength) : 0;
   if (select_link_width > 0)
   {
      _dsent_sel = new DSENTOpticalLink(
                link_frequency,                 // Core data rate (Right now, no serdes is assumed)
                link_frequency,                 // Link data rate (Right now, no serdes is assumed)
                waveguide_length / 1e3,         // Link length, convert to meters (m)
                num_receivers_per_wavelength,   // Number of readers
                num_receivers_per_wavelength,   // Maximum number of simultaneous readers
                select_link_width,              // Core flit width
                dsent_tuning_strategy,          // Ring tuning strategy
                dsent_laser_type,               // Laser type
                DSENTInterface::getSingleton()
            );
      _sel_link = true;
   }
   else _sel_link = false;
        
   // Static Power
   _static_power_leakage = _dsent_link->get_static_power_leakage();
   _static_power_laser = _dsent_link->get_static_power_laser();
   _static_power_heating = _dsent_link->get_static_power_heating();   
   if (_sel_link)
   {
      _static_power_leakage += _dsent_sel->get_static_power_leakage();
      _static_power_leakage += _dsent_sel->get_static_power_laser();
      _static_power_leakage += _dsent_sel->get_static_power_heating();
   }

   // Initialize dynamic energy counters
   initializeDynamicEnergyCounters();
}

OpticalLinkPowerModel::~OpticalLinkPowerModel()
{
    delete _dsent_link;
    if (_sel_link) delete _dsent_sel;
}

void
OpticalLinkPowerModel::initializeDynamicEnergyCounters()
{
   _total_dynamic_energy = 0.0;
}

void
OpticalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints)
{    
    // Some assertion checks based on ability of the laser
    // No support for arbitrary multicasts
    assert((num_endpoints == 1) || (num_endpoints == (signed int) _num_receivers_per_wavelength) || (num_endpoints == OpticalLinkModel::ENDPOINT_ALL));
    // If I want to send to more than 1 endpoint, broadcast better be supported
    assert((num_endpoints == 1) || (_laser_modes.broadcast));
    // If I am sending to one endpoint, I better be able to unicast (unless there is only 1 endpoint)
    assert((_num_receivers_per_wavelength == 1) || (num_endpoints > 1) || (_laser_modes.unicast));

    // Add dynamic energies
    _total_dynamic_energy += _dsent_link->calc_dynamic_energy(num_flits, num_endpoints);
    // Select network only needed during unicasts
    if (_sel_link && (num_endpoints == 1))
        _total_dynamic_energy += _dsent_sel->calc_dynamic_energy(num_flits, _num_receivers_per_wavelength);
}

const string
OpticalLinkPowerModel::getDSENTTuningStrategy(const string& carbon_tuning_strategy) const
{
   if (carbon_tuning_strategy == "athermal") return "AthermalWithTrim";
   else if (carbon_tuning_strategy == "full_thermal") return "FullThermal";
   else if (carbon_tuning_strategy == "thermal_reshuffle") return "ThermalWithBitReshuffle";
   else if (carbon_tuning_strategy == "electrical_assist") return "ElectricalAssistWithBitReshuffle";

   LOG_PRINT_ERROR("Unknown tuning strategy. Defaulting to athermal.");     
   return "AthermalWithTrim";
}

const string
OpticalLinkPowerModel::getDSENTLaserType(const string& carbon_laser_type) const
{
   if (carbon_laser_type == "standard") return "Standard";
   else if (carbon_laser_type == "throttled") return "Throttled";

   LOG_PRINT_ERROR("Unknown laser type. Defaulting to standard.");
   return "Standard";
}
