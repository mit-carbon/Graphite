#include <cmath>

#include "optical_network_link_model.h"
#include "simulator.h"
#include "config.h"
#include "log.h"

OpticalNetworkLinkModel::OpticalNetworkLinkModel(volatile float link_frequency, volatile double waveguide_length, UInt32 link_width):
   NetworkLinkModel(),
   _frequency(link_frequency),
   _waveguide_length(waveguide_length),
   _link_width(link_width)
{
   try
   {
      _num_receivers_per_wavelength = Sim()->getCfg()->getInt("link_model/optical/num_receiver_endpoints");

      // Delay Parameters
      _waveguide_delay_per_mm = Sim()->getCfg()->getFloat("link_model/optical/delay/waveguide_delay_per_mm");
      _e_o_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/delay/E-O_conversion");
      _o_e_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/delay/O-E_conversion");

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

   // _net_optical_link_delay is in clock cycles
   _net_optical_link_delay = (UInt64) (ceil( \
                                          (_waveguide_delay_per_mm * _waveguide_length) * _frequency + \
                                          _e_o_conversion_delay + \
                                          _o_e_conversion_delay \
                                        ));


}

OpticalNetworkLinkModel::~OpticalNetworkLinkModel()
{}

volatile double
OpticalNetworkLinkModel::getLaserPower()
{
   return ( _link_width * \
            ( \
              _laser_power \
            ) \
          );
}

volatile double
OpticalNetworkLinkModel::getRingTuningPower()
{
   return ( _link_width * \
            ( \
              _ring_tuning_power + \
              _num_receivers_per_wavelength * \
              ( \
                _ring_tuning_power \
              ) \
            ) \
          );
}

volatile double
OpticalNetworkLinkModel::getStaticPower()
{
   return ( _link_width * \
            ( \
               _laser_power + \
               _ring_tuning_power + _electrical_tx_static_power + _clock_static_power_tx + \
               _num_receivers_per_wavelength * \
               ( \
                  _ring_tuning_power + _electrical_rx_static_power + _clock_static_power_rx \
               ) \
            ) \
          );
}

volatile double
OpticalNetworkLinkModel::getDynamicEnergySender()
{
   return _total_dynamic_energy_sender;
}

volatile double
OpticalNetworkLinkModel::getDynamicEnergyReceiver()
{
   return _total_dynamic_energy_receiver;
}

void
OpticalNetworkLinkModel::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   _total_dynamic_energy_sender += (num_flits * (num_bit_flips * (_electrical_tx_dynamic_energy)));
   _total_dynamic_energy_receiver += (num_flits * (num_bit_flips * (_electrical_rx_dynamic_energy * _num_receivers_per_wavelength)));
}
