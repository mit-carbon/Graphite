#pragma once

#include "link_power_model.h"
#include "contrib/orion/orion.h"

class ElectricalLinkPowerModel : public LinkPowerModel
{
public:
   ElectricalLinkPowerModel(string link_type, float link_frequency, double link_length, UInt32 link_width);
   ~ElectricalLinkPowerModel();

   void updateDynamicEnergy(UInt32 num_flits);

private:
   OrionLink* _orion_link;
};
