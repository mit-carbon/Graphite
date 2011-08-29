#include <cmath>

#include "electrical_network_equalized_link_model.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

ElectricalNetworkEqualizedLinkModel::ElectricalNetworkEqualizedLinkModel(NetworkModel* model, volatile float link_frequency,
                                                                         volatile double link_length, UInt32 link_width)
   : ElectricalNetworkLinkModel(model, link_frequency, link_length, link_width)
{
   try
   {
      // Delay Parameters
      _wire_delay_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_equalized/delay/delay_per_mm");
      _tx_delay = Sim()->getCfg()->getInt("link_model/electrical_equalized/delay/tx_delay");
      _rx_delay = Sim()->getCfg()->getInt("link_model/electrical_equalized/delay/rx_delay");
      
      // Static Power Parameters
      _static_link_power_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_equalized/power/static/static_power_per_mm");
      _fixed_power = Sim()->getCfg()->getFloat("link_model/electrical_equalized/power/static/fixed_power");

      // Dynamic Power Parameters
      _dynamic_tx_energy_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_equalized/power/dynamic/dynamic_tx_energy_per_mm");
      _dynamic_rx_energy_per_mm = Sim()->getCfg()->getFloat("link_model/electrical_equalized/power/dynamic/dynamic_rx_energy_per_mm");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Unable to read equalized electrical link parameters from the cfg file");
   }

   _net_link_delay = (UInt64) ceil((_wire_delay_per_mm * _link_length) * _frequency +
                                    _tx_delay +
                                    _rx_delay);
   _total_static_power = _link_width * (_static_link_power_per_mm * _link_length + _fixed_power);
}

ElectricalNetworkEqualizedLinkModel::~ElectricalNetworkEqualizedLinkModel()
{}

void
ElectricalNetworkEqualizedLinkModel::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   _total_dynamic_energy += (num_flits * (num_bit_flips * (_dynamic_tx_energy_per_mm + _dynamic_rx_energy_per_mm) * _link_length));
}
