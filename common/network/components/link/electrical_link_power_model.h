#pragma once

#include <map>
#include <string>
using std::map;
using std::string;

#include "link_power_model.h"
#include "contrib/dsent/dsent_contrib.h"

class ElectricalLinkPowerModel : public LinkPowerModel
{
public:
   ElectricalLinkPowerModel(string link_type, double frequency, double voltage,
                            double link_length, UInt32 link_width);
   ~ElectricalLinkPowerModel();

   void setDVFS(double frequency, double voltage, const Time& curr_time);
   void computeEnergy(const Time& curr_time);
   void updateDynamicEnergy(UInt32 num_flits);

private:
   // DSENT model for the electrical link
   map<double, dsent_contrib::DSENTElectricalLink*> _dsent_link_map;
   dsent_contrib::DSENTElectricalLink* _dsent_link;
};
