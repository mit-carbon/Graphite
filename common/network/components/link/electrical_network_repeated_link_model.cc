#include <cmath>

#include "electrical_network_repeated_link_model.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

ElectricalNetworkRepeatedLinkModel::ElectricalNetworkRepeatedLinkModel(NetworkModel* model, volatile float link_frequency,
                                                                       volatile double link_length, UInt32 link_width)
   : ElectricalNetworkLinkModel(model, link_frequency, link_length, link_width)
{
   try
   {
      // Delay Parameters
      _delay_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_repeated/delay/delay_per_mm");

      // Static Power Parameters
      _static_link_power_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_repeated/power/static/static_power_per_mm");

      // Dynamic Power Parameters
      _dynamic_link_energy_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_repeated/power/dynamic/dynamic_energy_per_mm");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Unable to read repeated electrical link parameters from the cfg file");
   }
   
   _net_link_delay = (UInt64) ceil(_delay_per_mm * _link_length);
   _total_static_power = _link_width * _static_link_power_per_mm * _link_length;
}

ElectricalNetworkRepeatedLinkModel::~ElectricalNetworkRepeatedLinkModel()
{}

void
ElectricalNetworkRepeatedLinkModel::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   _total_dynamic_energy += (num_flits * (num_bit_flips * _dynamic_link_energy_per_mm * _link_length));
}
