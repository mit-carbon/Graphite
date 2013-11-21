#pragma once

#include <map>
using std::map;

#include "fixed_types.h"
#include "time_types.h"
#include "contrib/dsent/dsent_contrib.h"

class RouterPowerModel
{
public:
   RouterPowerModel(double frequency, double voltage, UInt32 num_input_ports, UInt32 num_output_ports,
                    UInt32 num_flits_per_port_buffer, UInt32 flit_width);
   ~RouterPowerModel();

   // Change voltage, frequency dynamically
   void setDVFS(double frequency, double voltage, const Time& curr_time);

   // Compute Energy upto this point 
   void computeEnergy(const Time& curr_time);
   // Get Dynamic Energy
   double getDynamicEnergy()
   {  
      return (_total_dynamic_energy_buffer + _total_dynamic_energy_crossbar + 
              _total_dynamic_energy_switch_allocator + _total_dynamic_energy_clock);
   }
   // Get Static Energy
   double getStaticEnergy()
   { 
      return _total_static_energy;
   }

   // Update Dynamic Energy
   void updateDynamicEnergy(UInt32 num_flits, UInt32 num_packets, UInt32 multicast_idx = 1);
 
private:
   UInt32 _num_input_ports;
   UInt32 _num_output_ports;

   map<double, dsent_contrib::DSENTRouter*> _dsent_router_map;
   dsent_contrib::DSENTRouter* _dsent_router;

   // Energy counters
   double _total_dynamic_energy_buffer;
   double _total_dynamic_energy_crossbar;
   double _total_dynamic_energy_switch_allocator;
   double _total_dynamic_energy_clock;
   double _total_static_energy;
   // Last energy compute time
   Time _last_energy_compute_time;

   void initializeEnergyCounters();
   double getStaticPower() const;

   void updateDynamicEnergyBufferWrite(UInt32 num_flits);
   void updateDynamicEnergyBufferRead(UInt32 num_flits);
   void updateDynamicEnergyCrossbar(UInt32 num_flits, UInt32 multicast_idx);
   void updateDynamicEnergySwitchAllocator(UInt32 num_requests_per_packet, UInt32 num_packets);
   void updateDynamicEnergyClock(UInt32 num_events);
};
