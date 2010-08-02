#pragma once

#include "electrical_network_link_model.h"
#include "contrib/orion/orion.h"

class ElectricalNetworkLinkModelOrion : public ElectricalNetworkLinkModel
{
public:
   ElectricalNetworkLinkModelOrion(LinkType link_type, volatile float link_frequency, \
         volatile double link_length, UInt32 link_width);
   ~ElectricalNetworkLinkModelOrion();

   UInt64 getDelay();
   volatile double getStaticPower();
   void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1);
   volatile double getDynamicEnergy() { return _total_dynamic_energy; }

private:
   OrionLink* _orion_link;
   volatile float _frequency;
   volatile double _total_dynamic_energy;
};
