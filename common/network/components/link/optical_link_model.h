#pragma once

#include "link_model.h"
#include "fixed_types.h"
#include "optical_link_power_model.h"

class NetPacket;
class NetworkModel;

class OpticalLinkModel : public LinkModel
{
public:
   OpticalLinkModel(NetworkModel* model, UInt32 num_receivers_per_wavelength,
                    float link_frequency, double waveguide_length, UInt32 link_width);
   ~OpticalLinkModel();

   // processPacket() called at every link
   void processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay);
   static const SInt32 ENDPOINT_ALL = -1;
   
   // Event counters specific to OpticalLink
   UInt64 getTotalUnicasts()     { return _total_link_unicasts; }
   UInt64 getTotalBroadcasts()   { return _total_link_broadcasts; }

private:
   OpticalLinkPowerModel* _power_model;

   // Delay parameters
   volatile double _waveguide_delay_per_mm;
   UInt64 _e_o_conversion_delay;
   UInt64 _o_e_conversion_delay;

   // Event Counters
   UInt64 _total_link_unicasts;
   UInt64 _total_link_broadcasts;
};
