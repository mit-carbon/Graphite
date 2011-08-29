#pragma once

#include "network_link_model.h"
#include "fixed_types.h"

class NetPacket;
class NetworkModel;

class OpticalNetworkLinkModel : public NetworkLinkModel
{
public:
   OpticalNetworkLinkModel(NetworkModel* model, UInt32 num_receivers_per_wavelength,
                           volatile float link_frequency, volatile double waveguide_length, UInt32 link_width);
   ~OpticalNetworkLinkModel();

   void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1);

   void processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay);
   static const SInt32 ENDPOINT_ALL = -1;
   UInt64 getTotalUnicasts() { return _total_link_unicasts; }
   UInt64 getTotalBroadcasts() { return _total_link_broadcasts; }

   // Public functions special to OpticalLink
   volatile double getLaserPower()              { return _total_laser_power; }
   volatile double getRingTuningPower()         { return _total_ring_tuning_power; }
   volatile double getDynamicEnergySender()     { return _total_dynamic_energy_sender; }
   volatile double getDynamicEnergyReceiver()   { return _total_dynamic_energy_receiver; }

private:
   UInt32 _num_receivers_per_wavelength;

   // Delay parameters
   volatile double _waveguide_delay_per_mm;
   UInt64 _e_o_conversion_delay;
   UInt64 _o_e_conversion_delay;

   // Static Power parameters
   volatile double _ring_tuning_power;
   volatile double _laser_power;
   volatile double _electrical_tx_static_power;
   volatile double _electrical_rx_static_power;
   volatile double _clock_static_power_tx;
   volatile double _clock_static_power_rx;

   // Dynamic Power parameters
   volatile double _electrical_tx_dynamic_energy;
   volatile double _electrical_rx_dynamic_energy;
   volatile double _total_dynamic_energy_sender;
   volatile double _total_dynamic_energy_receiver;

   // Aggregate Parameters
   volatile double _total_ring_tuning_power;
   volatile double _total_laser_power;

   // Event Counters
   UInt64 _total_link_unicasts;
   UInt64 _total_link_broadcasts;
};
