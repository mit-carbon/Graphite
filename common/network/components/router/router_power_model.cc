#include <cmath>
#include "router_power_model.h"
#include "log.h"

using namespace dsent_contrib;

RouterPowerModel::RouterPowerModel(float frequency, UInt32 num_input_ports, UInt32 num_output_ports,
                                   UInt32 num_flits_per_port_buffer, UInt32 flit_width)
   : _frequency(frequency)
   , _num_input_ports(num_input_ports)
   , _num_output_ports(num_output_ports)
   , _num_flits_per_port_buffer(num_flits_per_port_buffer)
   , _flit_width(flit_width)
{
   _dsent_router = new DSENTRouter(frequency, num_input_ports, num_output_ports, 1, 1,
                                   num_flits_per_port_buffer, flit_width, DSENTInterface::getSingleton());
   initializeCounters();
}

RouterPowerModel::~RouterPowerModel()
{
   delete _dsent_router;
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

   // Buffer write
   updateDynamicEnergyBufferWrite(num_flits);
   // Switch allocator
   updateDynamicEnergySwitchAllocator(contention, num_packets);
   // Buffer read
   updateDynamicEnergyBufferRead(num_flits);
   // Crossbar
   updateDynamicEnergyCrossbar(num_flits);
   // Clock - pretty much always on...not sure how to add the context of these variables
   updateDynamicEnergyClock(3 * num_flits + num_packets);
}

void
RouterPowerModel::updateDynamicEnergyBufferWrite(UInt32 num_flits)
{
   volatile double dynamic_energy_buffer = _dsent_router->calc_dynamic_energy_buf_write(num_flits);
   _total_dynamic_energy_buffer += dynamic_energy_buffer;
}

void
RouterPowerModel::updateDynamicEnergyBufferRead(UInt32 num_flits)
{
   volatile double dynamic_energy_buffer = _dsent_router->calc_dynamic_energy_buf_read(num_flits);
   _total_dynamic_energy_buffer += dynamic_energy_buffer;
}

void
RouterPowerModel::updateDynamicEnergyCrossbar(UInt32 num_flits)
{
   volatile double dynamic_energy_crossbar = _dsent_router->calc_dynamic_energy_xbar(num_flits);
   _total_dynamic_energy_crossbar += dynamic_energy_crossbar;
}

void
RouterPowerModel::updateDynamicEnergySwitchAllocator(UInt32 num_requests_per_packet, UInt32 num_packets)
{
   volatile double dynamic_energy_switch_allocator = _dsent_router->calc_dynamic_energy_sa(num_requests_per_packet);
   _total_dynamic_energy_switch_allocator += (num_packets * dynamic_energy_switch_allocator);
}

void
RouterPowerModel::updateDynamicEnergyClock(UInt32 num_events)
{
   volatile double dynamic_energy_clock = _dsent_router->calc_dynamic_energy_clock(num_events);
   _total_dynamic_energy_clock += dynamic_energy_clock;
}
