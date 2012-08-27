#include <cmath>
#include "router_power_model.h"
#include "log.h"

RouterPowerModel::RouterPowerModel(float frequency, UInt32 num_input_ports, UInt32 num_output_ports,
                                   UInt32 num_flits_per_port_buffer, UInt32 flit_width)
   : _frequency(frequency)
   , _num_input_ports(num_input_ports)
   , _num_output_ports(num_output_ports)
   , _num_flits_per_port_buffer(num_flits_per_port_buffer)
   , _flit_width(flit_width)
{
   _orion_router = new OrionRouter(frequency, num_input_ports, num_output_ports, 1, 1,
                                   num_flits_per_port_buffer, flit_width, OrionConfig::getSingleton());
   initializeCounters();
}

RouterPowerModel::~RouterPowerModel()
{
   delete _orion_router;
}

void
RouterPowerModel::initializeCounters()
{
   _total_dynamic_energy_buffer = 0;
   _total_dynamic_energy_crossbar = 0;
   _total_dynamic_energy_switch_allocator = 0;
   _total_dynamic_energy_clock = 0;
}

void
RouterPowerModel::updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets)
{
   UInt32 contention = ceil((1.0*_num_input_ports)/2);
   UInt32 num_bit_flips = _flit_width/2;

   // Buffer write
   updateDynamicEnergyBuffer(BufferAccess::WRITE, num_bit_flips, num_flits);
   // Switch allocator
   updateDynamicEnergySwitchAllocator(contention, num_packets);
   // Buffer read
   updateDynamicEnergyBuffer(BufferAccess::READ, num_bit_flips, num_flits);
   // Crossbar
   updateDynamicEnergyCrossbar(num_bit_flips, num_flits);
   // Clock - buffer write(num_flits), buffer_read(num_flits), crossbar(num_flits), switch allocator(num_packets)
   // assuming all these happen in separate clock cycles
   updateDynamicEnergyClock(3*num_flits+num_packets);
}

void
RouterPowerModel::updateDynamicEnergyBuffer(BufferAccess::Type buffer_access_type, UInt32 num_bit_flips, UInt32 num_flits)
{
   bool is_read = (buffer_access_type == BufferAccess::READ) ? true : false;
   volatile double dynamic_energy_buffer = _orion_router->calc_dynamic_energy_buf(is_read);
   _total_dynamic_energy_buffer += (num_flits * dynamic_energy_buffer);
}

void
RouterPowerModel::updateDynamicEnergyCrossbar(UInt32 num_bit_flips, UInt32 num_flits)
{
   volatile double dynamic_energy_crossbar = _orion_router->calc_dynamic_energy_xbar();
   _total_dynamic_energy_crossbar += (num_flits * dynamic_energy_crossbar);
}

void
RouterPowerModel::updateDynamicEnergySwitchAllocator(UInt32 num_requests, UInt32 num_packets)
{
   volatile double dynamic_energy_switch_allocator = _orion_router->calc_dynamic_energy_global_sw_arb(num_requests);
   _total_dynamic_energy_switch_allocator += (num_packets * dynamic_energy_switch_allocator);
}

void
RouterPowerModel::updateDynamicEnergyClock(UInt32 num_events)
{
   volatile double dynamic_energy_clock = _orion_router->calc_dynamic_energy_clock();
   _total_dynamic_energy_clock += (num_events * dynamic_energy_clock);
}
