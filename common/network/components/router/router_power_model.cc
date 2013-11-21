#include <cmath>
#include "router_power_model.h"
#include "dvfs_manager.h"
#include "log.h"

using namespace dsent_contrib;

RouterPowerModel::RouterPowerModel(double frequency, double voltage, UInt32 num_input_ports, UInt32 num_output_ports,
                                   UInt32 num_flits_per_port_buffer, UInt32 flit_width)
   : _num_input_ports(num_input_ports)
   , _num_output_ports(num_output_ports)
{
   // Create the DSENT router models
   const DVFSManager::DVFSLevels& dvfs_levels = DVFSManager::getDVFSLevels();
   for (DVFSManager::DVFSLevels::const_iterator it = dvfs_levels.begin(); it != dvfs_levels.end(); it++)
   {
      double current_voltage = (*it).first;
      double current_frequency = (*it).second;

      // Create DSENT router (and) save for future use
      // DSENT expects voltage in volts (V)
      // DSENT expects frequency in hertz (Hz)
      _dsent_router_map[current_voltage] =  new DSENTRouter(current_frequency * 1e9, current_voltage,
                                                            num_input_ports, num_output_ports,
                                                            1, 1,
                                                            num_flits_per_port_buffer, flit_width,
                                                            DSENTInterface::getSingleton());
   }
   
   // Initialize the current DSENT router model
   _dsent_router = _dsent_router_map[voltage];
   
   // Initialize energy counters
   initializeEnergyCounters();
}

RouterPowerModel::~RouterPowerModel()
{
   delete _dsent_router;
}

void
RouterPowerModel::initializeEnergyCounters()
{
   _total_dynamic_energy_buffer = 0;
   _total_dynamic_energy_crossbar = 0;
   _total_dynamic_energy_switch_allocator = 0;
   _total_dynamic_energy_clock = 0;
   _total_static_energy = 0;
   _last_energy_compute_time = Time(0);
}

void
RouterPowerModel::updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets, UInt32 multicast_idx)
{
   LOG_ASSERT_ERROR(multicast_idx >= 1 && multicast_idx <= _num_output_ports,
                    "Multicast idx should be between >= 1 (and) <= %u. Now, it is %u",
                    _num_output_ports, multicast_idx);

   UInt32 contention = ceil((1.0*_num_input_ports)/2);

   // Buffer write
   updateDynamicEnergyBufferWrite(num_flits);
   // Switch allocator
   updateDynamicEnergySwitchAllocator(contention, num_packets);
   // Buffer read
   updateDynamicEnergyBufferRead(num_flits);
   // Crossbar
   updateDynamicEnergyCrossbar(num_flits, multicast_idx);
   // Clock - pretty much always on...not sure how to add the context of these variables
   updateDynamicEnergyClock(3 * num_flits + num_packets);
}

void
RouterPowerModel::updateDynamicEnergyBufferWrite(UInt32 num_flits)
{
   _total_dynamic_energy_buffer += _dsent_router->calc_dynamic_energy_buf_write(num_flits);
}

void
RouterPowerModel::updateDynamicEnergyBufferRead(UInt32 num_flits)
{
   _total_dynamic_energy_buffer += _dsent_router->calc_dynamic_energy_buf_read(num_flits);
}

void
RouterPowerModel::updateDynamicEnergyCrossbar(UInt32 num_flits, UInt32 multicast_idx)
{
   _total_dynamic_energy_crossbar += _dsent_router->calc_dynamic_energy_xbar(num_flits, multicast_idx);
}

void
RouterPowerModel::updateDynamicEnergySwitchAllocator(UInt32 num_requests_per_packet, UInt32 num_packets)
{
   _total_dynamic_energy_switch_allocator += _dsent_router->calc_dynamic_energy_sa(num_requests_per_packet);
}

void
RouterPowerModel::updateDynamicEnergyClock(UInt32 num_events)
{
   _total_dynamic_energy_clock += _dsent_router->calc_dynamic_energy_clock(num_events);
}

void
RouterPowerModel::setDVFS(double frequency, double voltage, const Time& curr_time)
{
   LOG_PRINT("Router setDVFS[Frequency(%g), Voltage(%g), Time(%llu ns)] begin", frequency, voltage, curr_time.toNanosec());
   // Compute leakage/dynamic energy
   computeEnergy(curr_time);

   // Set new DSENT router model
   _dsent_router = _dsent_router_map[voltage];
   LOG_ASSERT_ERROR(_dsent_router, "Could not find DSENT Router model for voltage(%g)", voltage);
   LOG_PRINT("Router setDVFS[Frequency(%g), Voltage(%g), Time(%llu ns)] end", frequency, voltage, curr_time.toNanosec());
}

void
RouterPowerModel::computeEnergy(const Time& curr_time)
{
   // Compute the interval between current time and time when energy was last computed
   double time_interval = (curr_time - _last_energy_compute_time).toSec();
   // Increment total static energy
   _total_static_energy += (getStaticPower() * time_interval);
   // Set _last_energy_compute_time to curr_time
   _last_energy_compute_time = curr_time;
}

double
RouterPowerModel::getStaticPower() const
{
   return _dsent_router->get_static_power_buf() + _dsent_router->get_static_power_xbar() +
          _dsent_router->get_static_power_sa() + _dsent_router->get_static_power_clock();
}
