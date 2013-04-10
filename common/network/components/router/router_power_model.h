#pragma once

#include "fixed_types.h"
#include "contrib/dsent/dsent_contrib.h"

class RouterPowerModel
{
public:
   RouterPowerModel(double frequency, UInt32 num_input_ports, UInt32 num_output_ports, UInt32 num_flits_per_port_buffer, UInt32 flit_width);
   ~RouterPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets, UInt32 multicast_idx = 1);
   
   // Get Dynamic Energy
   volatile double getDynamicEnergy()
   {  
      return (_total_dynamic_energy_buffer + _total_dynamic_energy_crossbar + 
              _total_dynamic_energy_switch_allocator + _total_dynamic_energy_clock);
   }
   volatile double getDynamicEnergyBuffer()           { return _total_dynamic_energy_buffer;             }
   volatile double getDynamicEnergyCrossbar()         { return _total_dynamic_energy_crossbar;           }
   volatile double getDynamicEnergySwitchAllocator()  { return _total_dynamic_energy_switch_allocator;   }
   volatile double getDynamicEnergyClock()            { return _total_dynamic_energy_clock;              }
   
   // Static Power
   volatile double getStaticPowerBuffer()             { return _dsent_router->get_static_power_buf();    }
   volatile double getStaticPowerBufferCrossbar()     { return _dsent_router->get_static_power_xbar();   }
   volatile double getStaticPowerSwitchAllocator()    { return _dsent_router->get_static_power_sa();     }
   volatile double getStaticPowerClock()              { return _dsent_router->get_static_power_clock();  }
   volatile double getStaticPower()
   {
      return (_dsent_router->get_static_power_buf() + _dsent_router->get_static_power_xbar() +
              _dsent_router->get_static_power_sa() + _dsent_router->get_static_power_clock());
   }

private:
   volatile double _frequency;
   UInt32 _num_input_ports;
   UInt32 _num_output_ports;
   UInt32 _num_flits_per_port_buffer;
   UInt32 _flit_width;

   dsent_contrib::DSENTRouter* _dsent_router;

   volatile double _total_dynamic_energy_buffer;
   volatile double _total_dynamic_energy_crossbar;
   volatile double _total_dynamic_energy_switch_allocator;
   volatile double _total_dynamic_energy_clock;

   void initializeCounters();
   
   void updateDynamicEnergyBufferWrite(UInt32 num_flits);
   void updateDynamicEnergyBufferRead(UInt32 num_flits);
   void updateDynamicEnergyCrossbar(UInt32 num_flits, UInt32 multicast_idx);
   void updateDynamicEnergySwitchAllocator(UInt32 num_requests_per_packet, UInt32 num_packets);
   void updateDynamicEnergyClock(UInt32 num_events);
};
