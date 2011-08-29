#include <cmath>

#include "optical_network_link_model.h"
#include "simulator.h"
#include "config.h"
#include "network_model.h"
#include "network.h"
#include "log.h"

OpticalNetworkLinkModel::OpticalNetworkLinkModel(NetworkModel* model, UInt32 num_receivers_per_wavelength,
                                                 volatile float link_frequency, volatile double waveguide_length, UInt32 link_width)
   : NetworkLinkModel(model, link_frequency, waveguide_length, link_width)
   , _num_receivers_per_wavelength(num_receivers_per_wavelength)
   , _total_link_unicasts(0)
   , _total_link_broadcasts(0)
{
   try
   {
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
   _net_link_delay = (UInt64) (ceil( (_waveguide_delay_per_mm * _link_length) * _frequency + 
                                      _e_o_conversion_delay +
                                      _o_e_conversion_delay) );

   // Static Power
   _total_ring_tuning_power = _link_width * (_ring_tuning_power * _num_receivers_per_wavelength);
   _total_laser_power = _link_width * _laser_power;
   _total_static_power = _total_laser_power + _total_ring_tuning_power +
                         (_link_width * _electrical_tx_static_power) +
                         (_link_width * _num_receivers_per_wavelength * _electrical_rx_static_power) +
                         (_link_width * _clock_static_power_tx) +
                         (_link_width * _num_receivers_per_wavelength * _clock_static_power_rx);
}

OpticalNetworkLinkModel::~OpticalNetworkLinkModel()
{}

void
OpticalNetworkLinkModel::updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits)
{
   _total_dynamic_energy_sender += (num_flits * (num_bit_flips * (_electrical_tx_dynamic_energy)));
   _total_dynamic_energy_receiver += (num_flits * (num_bit_flips * (_electrical_rx_dynamic_energy * _num_receivers_per_wavelength)));
   _total_dynamic_energy = _total_dynamic_energy_sender + _total_dynamic_energy_receiver;
}

void
OpticalNetworkLinkModel::processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   zero_load_delay += _net_link_delay;
  
   // Increment Event Counters
   UInt32 pkt_length = _model->getModeledLength(pkt);
   SInt32 num_flits = _model->computeNumFlits(pkt_length);

   if (num_endpoints == ENDPOINT_ALL)
      _total_link_broadcasts += num_flits;
   else if (num_endpoints == 1)
      _total_link_unicasts += num_flits;
   else
      LOG_PRINT_ERROR("Unrecognized number of endpoints(%i)", num_endpoints);
}
