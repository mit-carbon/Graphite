#pragma once

#include "link_power_model.h"
#include "contrib/dsent/dsent_contrib.h"

class ElectricalLinkPowerModel : public LinkPowerModel
{

using dsent_contrib::DSENTElectricalLink;

public:
   ElectricalLinkPowerModel(string link_type, float link_frequency, double link_length, UInt32 link_width);
   ~ElectricalLinkPowerModel();

   void updateDynamicEnergy(UInt32 num_flits);

private:
   DSENTElectricalLink* _dsent_link;
};
