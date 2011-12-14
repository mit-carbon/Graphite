#include "electrical_network_link_model.h"
#include "log.h"

ElectricalNetworkLinkModel::ElectricalNetworkLinkModel(string link_type, float link_frequency, double link_length, UInt32 link_width)
   : _total_dynamic_energy(0)
{
   LOG_ASSERT_ERROR(link_type == "electrical_repeated", "Orion only supports electrical_repeated link models currently");
   // Link Length is passed in meters(m)
   _orion_link = new OrionLink(link_frequency, link_length / 1000, link_width, OrionConfig::getSingleton());
}

ElectricalNetworkLinkModel::~ElectricalNetworkLinkModel()
{
   delete _orion_link;
}

volatile double
ElectricalNetworkLinkModel::getStaticPower()
{
   return _orion_link->get_static_power();
}

void
ElectricalNetworkLinkModel::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   volatile double dynamic_energy = _orion_link->calc_dynamic_energy(num_bit_flips);
   _total_dynamic_energy += (num_flits * dynamic_energy);
}
