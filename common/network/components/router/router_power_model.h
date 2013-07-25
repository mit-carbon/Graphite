#pragma once

#include "fixed_types.h"
#include "contrib/dsent/dsent_contrib.h"

class RouterPowerModel
{
public:
   RouterPowerModel(float frequency, UInt32 num_input_ports, UInt32 num_output_ports, UInt32 num_flits_per_port_buffer, UInt32 flit_width);
   ~RouterPowerModel();

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets, UInt32 multicast_idx = 1);
   
   // Get Dynamic Energy
   double getDynamicEnergy()
   {  
      return (_total_dynamic_energy_buffer + _total_dynamic_energy_crossbar + 
              _total_dynamic_energy_switch_allocator + _total_dynamic_energy_clock);
   }
   double getDynamicEnergyBuffer()           { return _total_dynamic_energy_buffer;             }
   double getDynamicEnergyCrossbar()         { return _total_dynamic_energy_crossbar;           }
   double getDynamicEnergySwitchAllocator()  { return _total_dynamic_energy_switch_allocator;   }
   double getDynamicEnergyClock()            { return _total_dynamic_energy_clock;              }
   
   // Static Power
   double getStaticPowerBuffer()             { return _dsent_router->get_static_power_buf();    }
   double getStaticPowerBufferCrossbar()     { return _dsent_router->get_static_power_xbar();   }
   double getStaticPowerSwitchAllocator()    { return _dsent_router->get_static_power_sa();     }
   double getStaticPowerClock()              { return _dsent_router->get_static_power_clock();  }
   double getStaticPower()
   {
      return (_dsent_router->get_static_power_buf() + _dsent_router->get_static_power_xbar() +
              _dsent_router->get_static_power_sa() + _dsent_router->get_static_power_clock());
   }

private:
   float _frequency;
   UInt32 _num_input_ports;
   UInt32 _num_output_ports;
   UInt32 _num_flits_per_port_buffer;
   UInt32 _flit_width;

   dsent_contrib::DSENTRouter* _dsent_router;

   double _total_dynamic_energy_buffer;
   double _total_dynamic_energy_crossbar;
   double _total_dynamic_energy_switch_allocator;
   double _total_dynamic_energy_clock;

   void initializeCounters();
   
   void updateDynamicEnergyBufferWrite(UInt32 num_flits);
   void updateDynamicEnergyBufferRead(UInt32 num_flits);
   void updateDynamicEnergyCrossbar(UInt32 num_flits, UInt32 multicast_idx);
   void updateDynamicEnergySwitchAllocator(UInt32 num_requests_per_packet, UInt32 num_packets);
   void updateDynamicEnergyClock(UInt32 num_events);
};
