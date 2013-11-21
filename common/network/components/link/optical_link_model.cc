#include <cmath>

#include "optical_link_power_model.h"
#include "optical_link_model.h"
#include "simulator.h"
#include "config.h"
#include "network_model.h"
#include "network.h"
#include "utils.h"
#include "log.h"

OpticalLinkModel::OpticalLinkModel(NetworkModel* model, UInt32 num_readers_per_wavelength,
                                   double frequency, double voltage, double waveguide_length, UInt32 link_width)
   : LinkModel(model)
   , _power_model(NULL)
   , _num_readers_per_wavelength(num_readers_per_wavelength)
   , _total_link_unicasts(0)
   , _total_link_broadcasts(0)
{
   double waveguide_delay_per_mm = 0.0;
   double e_o_conversion_delay = 0.0;
   double o_e_conversion_delay = 0.0;
   try
   {
      // Delay Parameters
      waveguide_delay_per_mm = Sim()->getCfg()->getFloat("link_model/optical/waveguide_delay_per_mm");
      e_o_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/E-O_conversion_delay");
      o_e_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/O-E_conversion_delay");
      // Laser
      _laser_modes = parseLaserModes(Sim()->getCfg()->getString("link_model/optical/laser_modes"));
      _laser_type = parseLaserType(Sim()->getCfg()->getString("link_model/optical/laser_type"));
      // Ring tuning
      _ring_tuning_strategy = Sim()->getCfg()->getString("link_model/optical/ring_tuning_strategy");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read optical link parameters from the cfg file");
   }

   if (_laser_type == STANDARD)
   {
      LOG_ASSERT_ERROR (_laser_modes.unicast ^ _laser_modes.broadcast,
                        "Laser type: (standard), Laser modes: must be only one of (unicast) OR (broadcast)");  
   }
   else if (_laser_type == THROTTLED)
   {
      LOG_ASSERT_ERROR (_laser_modes.unicast || _laser_modes.broadcast,
                        "Laser type: (throttled), Laser modes: both (unicast) AND (broadcast) disabled");  
   }

   // _net_optical_link_delay is in clock cycles (in terms of network frequency)
   _net_link_delay = (UInt64) (ceil( (waveguide_delay_per_mm * waveguide_length) * frequency + 
                                      e_o_conversion_delay +
                                      o_e_conversion_delay) );

   // Power Model
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model = new OpticalLinkPowerModel(_laser_modes, _laser_type, _ring_tuning_strategy,
                                               _num_readers_per_wavelength, frequency, voltage,
                                               waveguide_length, link_width);
}

OpticalLinkModel::~OpticalLinkModel()
{
   if (Config::getSingleton()->getEnablePowerModeling())
      delete _power_model;
}

OpticalLinkModel::LaserModes
OpticalLinkModel::parseLaserModes(string laser_modes_str)
{
   vector<string> laser_modes_vec;
   splitIntoTokens(laser_modes_str, laser_modes_vec, ",");
  
   LaserModes laser_modes;
   for (vector<string>::iterator it = laser_modes_vec.begin(); it != laser_modes_vec.end(); it++)
   {
      if ((*it) == "unicast")
         laser_modes.unicast = true;
      else if ((*it) == "broadcast")
         laser_modes.broadcast = true;
      else
         LOG_PRINT_ERROR("Unrecognzied Laser Mode(%s)", (*it).c_str());
   }
   return laser_modes;
}

OpticalLinkModel::LaserType
OpticalLinkModel::parseLaserType(string laser_type_str)
{
   if (laser_type_str == "standard")
      return STANDARD;
   else if (laser_type_str == "throttled")
      return THROTTLED;
   else
   {
      LOG_PRINT_ERROR("Unrecognzied laser type: %s", laser_type_str.c_str());
      return (LaserType) -1;
   }
}

void
OpticalLinkModel::processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   zero_load_delay += _net_link_delay;
  
   // Increment event counters
   UInt32 pkt_length = _model->getModeledLength(pkt); // pkt_length is in bits
   SInt32 num_flits = _model->computeNumFlits(pkt_length);

   // Change num_endpoints -> _num_readers_per_wavelength (if) num_endpoints = ENDPOINT_ALL
   if (num_endpoints == ENDPOINT_ALL)
   {
      LOG_ASSERT_ERROR(_laser_modes.broadcast, "Broadcast mode not enabled. Num endpoints should be 1: (%i)", num_endpoints);
      _total_link_broadcasts += num_flits;
      num_endpoints = _num_readers_per_wavelength;
   }
   else if (num_endpoints == 1)
   {
      // If unicast mode is present, use only laser energy for unicast
      // If unicast mode is NOT present, use laser energy for broadcast
      if (_laser_modes.unicast)
      {
         _total_link_unicasts += num_flits;
      }
      else // (_laser_modes.broadcast)
      {
         _total_link_broadcasts += num_flits;
         num_endpoints = _num_readers_per_wavelength;
      }
   }
   else
   {
      // No support for arbitrary multicasts
      LOG_PRINT_ERROR("Num endpoints is neither 1 nor ENDPOINT_ALL: (%i)", num_endpoints);
   }

   // Update dynamic energy counters
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->updateDynamicEnergy(num_flits, num_endpoints);
}
