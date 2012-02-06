#include <cmath>

#include "optical_link_model.h"
#include "optical_link_power_model.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "log.h"

OpticalLinkPowerModel::OpticalLinkPowerModel(LaserModes& laser_modes, UInt32 num_receivers_per_wavelength,
                                             float link_frequency, double waveguide_length, UInt32 link_width)
   : LinkPowerModel(link_frequency, waveguide_length, link_width)
   , _laser_modes(laser_modes)
   , _num_receivers_per_wavelength(num_receivers_per_wavelength)
{
   try
   {
      // Ring Tuning and Laser Power
      _ring_tuning_power = Sim()->getCfg()->getFloat("link_model/optical/power/ring_tuning_power");
      _laser_power_per_receiver = Sim()->getCfg()->getFloat("link_model/optical/power/laser_power_per_receiver");
      
      // Static Power parameters
      _electrical_tx_static_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/electrical_tx_power");
      _electrical_rx_static_power = Sim()->getCfg()->getFloat("link_model/optical/power/static/electrical_rx_power");

      // Dynamic Power parameters
      _electrical_tx_dynamic_energy = Sim()->getCfg()->getFloat("link_model/optical/power/dynamic/electrical_tx_energy");
      _electrical_rx_dynamic_energy = Sim()->getCfg()->getFloat("link_model/optical/power/dynamic/electrical_rx_energy");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read optical link parameters from the cfg file");
   }

   // Ring Tuning Power
   double total_static_ring_tuning_power_select_link = _ring_tuning_power * ceilLog2(_num_receivers_per_wavelength) *
                                                       _num_receivers_per_wavelength;
   double total_static_ring_tuning_power_data_link = _ring_tuning_power * _link_width * _num_receivers_per_wavelength;
   _total_static_ring_tuning_power = total_static_ring_tuning_power_select_link + total_static_ring_tuning_power_data_link;

   // Laser Power
   double total_static_laser_power_select_link = 0.0;
   double total_static_laser_power_data_link = 0.0;
   if (!_laser_modes.idle)
   {
      total_static_laser_power_select_link = _laser_power_per_receiver * ceilLog2(_num_receivers_per_wavelength) *
                                             _num_receivers_per_wavelength;
      total_static_laser_power_data_link = _link_width * _laser_power_per_receiver * _num_receivers_per_wavelength;
   }
   _total_static_laser_power = total_static_laser_power_select_link + total_static_laser_power_data_link;

   // Electrical Static Tx Power
   double total_static_power_tx_select_link = _electrical_tx_static_power * ceilLog2(_num_receivers_per_wavelength);
   double total_static_power_tx_data_link = _electrical_tx_static_power * _link_width;
   _total_static_power_tx = total_static_power_tx_select_link + total_static_power_tx_data_link;

   // Electrical Static Tx Power
   double total_static_power_rx_select_link = _electrical_rx_static_power * ceilLog2(_num_receivers_per_wavelength) *
                                              _num_receivers_per_wavelength;
   double total_static_power_rx_data_link = _electrical_rx_static_power * _link_width * _num_receivers_per_wavelength;
   _total_static_power_rx = total_static_power_rx_select_link + total_static_power_rx_data_link;
   
   // Total Static Power  
   _total_static_power = _total_static_laser_power + _total_static_ring_tuning_power +
                         _total_static_power_tx + _total_static_power_rx;

   // Initialize dynamic energy counters
   initializeDynamicEnergyCounters();
}

OpticalLinkPowerModel::~OpticalLinkPowerModel()
{}

void
OpticalLinkPowerModel::initializeDynamicEnergyCounters()
{
   _total_dynamic_laser_energy = 0.0;
   _total_dynamic_energy_tx = 0.0;
   _total_dynamic_energy_rx = 0.0;
}

void
OpticalLinkPowerModel::updateDynamicEnergy(UInt32 num_flits, SInt32 num_endpoints)
{
   SInt32 num_receivers = ((num_endpoints == OpticalLinkModel::ENDPOINT_ALL) || (!_laser_modes.unicast)) ? _num_receivers_per_wavelength : 1;
  
   volatile double laser_energy_select_link = 0.0;
   volatile double laser_energy_data_link = 0.0;
   if (_laser_modes.idle)
   {
      laser_energy_select_link = _laser_power_per_receiver * ceilLog2(_num_receivers_per_wavelength) *
                                 _num_receivers_per_wavelength / (_frequency * 1e9); 
      laser_energy_data_link = _laser_power_per_receiver * num_flits * _link_width * num_receivers / (_frequency * 1e9);
   }
   volatile double laser_energy = laser_energy_select_link + laser_energy_data_link;
   _total_dynamic_laser_energy += laser_energy;

   UInt32 num_bit_flips_select_link = ceilLog2(_num_receivers_per_wavelength) / 2;
   UInt32 num_bit_flips_data_link = _link_width / 2;
   
   volatile double tx_energy_select_link = _electrical_tx_dynamic_energy * num_bit_flips_select_link;
   volatile double tx_energy_data_link = _electrical_tx_dynamic_energy * num_flits * num_bit_flips_data_link;
   volatile double tx_energy = tx_energy_select_link + tx_energy_data_link;
   _total_dynamic_energy_tx += tx_energy;

   volatile double rx_energy_select_link = _electrical_rx_dynamic_energy * num_bit_flips_select_link * _num_receivers_per_wavelength;
   volatile double rx_energy_data_link = _electrical_rx_dynamic_energy * num_flits * num_bit_flips_data_link * num_receivers;
   volatile double rx_energy = rx_energy_select_link + rx_energy_data_link;
   _total_dynamic_energy_rx += rx_energy;

   _total_dynamic_energy += (laser_energy + tx_energy + rx_energy);
}
