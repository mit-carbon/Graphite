#include "electrical_link_power_model.h"
#include "dvfs_manager.h"
#include "log.h"

using namespace dsent_contrib;

ElectricalLinkPowerModel::ElectricalLinkPowerModel(string link_type, double frequency, double voltage,
                                                   double link_length, UInt32 link_width)
   : LinkPowerModel()
{
   LOG_ASSERT_ERROR(link_type == "electrical_repeated", "DSENT only supports electrical_repeated link models currently");
  
   const DVFSManager::DVFSLevels& dvfs_levels = DVFSManager::getDVFSLevels();
   for (DVFSManager::DVFSLevels::const_iterator it = dvfs_levels.begin(); it != dvfs_levels.end(); it++)
   {
      double current_voltage = (*it).first;
      double current_frequency = (*it).second;

      // DSENT expects link length to be in meters(m)
      // DSENT expects link frequency to be in hertz (Hz)
      _dsent_link_map[current_voltage] = new DSENTElectricalLink(current_frequency * 1e9, current_voltage,
                                                                 link_length / 1000, link_width,
                                                                 DSENTInterface::getSingleton());
   }

   // Set current DSENT electrical link model
   _dsent_link = _dsent_link_map[voltage];
}

ElectricalLinkPowerModel::~ElectricalLinkPowerModel()
{
   delete _dsent_link;
}

void
ElectricalLinkPowerModel::setDVFS(double frequency, double voltage, const Time& curr_time)
{
   LOG_PRINT("Electrical-Link setDVFS[Frequency(%g), Voltage(%g), Time(%llu ns)] begin", frequency, voltage, curr_time.toNanosec());
   // Compute leakage/dynamic energy
   computeEnergy(curr_time);
   
   // Set new DSENT electrical link model
   _dsent_link = _dsent_link_map[voltage];
   LOG_ASSERT_ERROR(_dsent_link, "Could not find DSENT Electrical Link model for voltage(%g)", voltage);
   LOG_PRINT("Electrical-Link setDVFS[Frequency(%g), Voltage(%g), Time(%llu ns)] end", frequency, voltage, curr_time.toNanosec());
}

void
ElectricalLinkPowerModel::computeEnergy(const Time& curr_time)
{
   // Compute the interval between current time and time when energy was last computed
   double time_interval = (curr_time - _last_energy_compute_time).toSec();
   // Increment total static energy
   _total_static_energy += _dsent_link->get_static_power() * time_interval;
   // Set _last_energy_compute_time to curr_time
   _last_energy_compute_time = curr_time;
}

void
ElectricalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits)
{
   _total_dynamic_energy += _dsent_link->calc_dynamic_energy(num_flits);
}
