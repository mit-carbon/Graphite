#include "electrical_link_power_model.h"
#include "log.h"

ElectricalLinkPowerModel::ElectricalLinkPowerModel(string link_type, float link_frequency, double link_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, link_length, link_width)
{
   LOG_ASSERT_ERROR(link_type == "electrical_repeated", "Orion only supports electrical_repeated link models currently");
   // Link Length is passed in meters(m)
   _orion_link = new OrionLink(link_frequency, link_length / 1000, link_width, OrionConfig::getSingleton());

   // Static Power
   _total_static_power = _orion_link->get_static_power();
}

ElectricalLinkPowerModel::~ElectricalLinkPowerModel()
{
   delete _orion_link;
}

void
ElectricalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits)
{
   UInt32 num_bit_flips = _link_width / 2;
   volatile double dynamic_energy = _orion_link->calc_dynamic_energy(num_bit_flips);
   _total_dynamic_energy += (num_flits * dynamic_energy);
}
