#include "electrical_link_power_model.h"
#include "log.h"

using namespace dsent_contrib;

ElectricalLinkPowerModel::ElectricalLinkPowerModel(string link_type, float link_frequency, double link_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, link_length, link_width)
{
   LOG_ASSERT_ERROR(link_type == "electrical_repeated", "DSENT only supports electrical_repeated link models currently");
   // DSENT expects link length to be in meters(m)
   // DSENT expects link frequency to be in hertz (Hz)
   _dsent_link = new DSENTElectricalLink(link_frequency * 1e9, link_length / 1000, link_width, DSENTInterface::getSingleton());

   // Static Power
   _total_static_power = _dsent_link->get_static_power();
}

ElectricalLinkPowerModel::~ElectricalLinkPowerModel()
{
   delete _dsent_link;
}

void
ElectricalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits)
{
   _total_dynamic_energy += _dsent_link->calc_dynamic_energy(num_flits);
}
