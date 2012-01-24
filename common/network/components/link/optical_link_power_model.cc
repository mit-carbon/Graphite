#include <cmath>

#include "optical_link_power_model.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

OpticalLinkPowerModel::OpticalLinkPowerModel(UInt32 num_receivers_per_wavelength, float link_frequency,
                                             double waveguide_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, waveguide_length, link_width)
   , _num_receivers_per_wavelength(num_receivers_per_wavelength)
{
   try
   {
      // Static Power parameters
      _ring_tuning_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/ring_tuning_power");
      _laser_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/laser_power");
      _electrical_tx_static_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/electrical_tx_power");
      _electrical_rx_static_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/electrical_rx_power");
      _clock_static_power_tx = Sim()->getCfg()->getFloat("link_model/optical/power/fixed/clock_power_tx");
      _clock_static_power_rx = Sim()->getCfg()->getFloat("link_model/optical/power/fixed/clock_power_rx");

      // Dynamic Power parameters
      _electrical_tx_dynamic_energy = Sim()->getCfg()->getFloat("link_model/optical/power/dynamic/electrical_tx_energy");
      _electrical_rx_dynamic_energy = Sim()->getCfg()->getFloat("link_model/optical/power/dynamic/electrical_rx_energy");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read optical link parameters from the cfg file");
   }

   // Static Power
   _total_ring_tuning_power = _link_width * (_ring_tuning_power * (1 + _num_receivers_per_wavelength));
   _total_laser_power = _link_width * _laser_power;
   _total_static_power = _total_laser_power + _total_ring_tuning_power +
                         (_link_width * _electrical_tx_static_power) +
                         (_link_width * _num_receivers_per_wavelength * _electrical_rx_static_power) +
                         (_link_width * _clock_static_power_tx) +
                         (_link_width * _num_receivers_per_wavelength * _clock_static_power_rx);
}

OpticalLinkPowerModel::~OpticalLinkPowerModel()
{}

void
OpticalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits)
{
   UInt32 num_bit_flips = _link_width / 2;
   _total_dynamic_energy_sender += (num_flits * (num_bit_flips * (_electrical_tx_dynamic_energy)));
   _total_dynamic_energy_receiver += (num_flits * (num_bit_flips * (_electrical_rx_dynamic_energy * _num_receivers_per_wavelength)));
   _total_dynamic_energy = _total_dynamic_energy_sender + _total_dynamic_energy_receiver;
}
