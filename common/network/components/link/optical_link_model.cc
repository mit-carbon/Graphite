#include <cmath>

#include "optical_link_model.h"
#include "optical_link_power_model.h"
#include "simulator.h"
#include "config.h"
#include "network_model.h"
#include "network.h"
#include "log.h"

OpticalLinkModel::OpticalLinkModel(NetworkModel* model, LaserModes& laser_modes, UInt32 num_receivers_per_wavelength,
                                   float link_frequency, double waveguide_length, UInt32 link_width)
   : LinkModel(model, link_frequency, waveguide_length, link_width)
   , _power_model(NULL)
   , _laser_modes(laser_modes)
   , _total_link_unicasts(0)
   , _total_link_broadcasts(0)
{
   try
   {
      // Delay Parameters
      _waveguide_delay_per_mm = Sim()->getCfg()->getFloat("link_model/optical/delay/waveguide_delay_per_mm");
      _e_o_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/delay/E-O_conversion");
      _o_e_conversion_delay = Sim()->getCfg()->getInt("link_model/optical/delay/O-E_conversion");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read optical link parameters from the cfg file");
   }

   // _net_optical_link_delay is in clock cycles (in terms of network frequency)
   _net_link_delay = (UInt64) (ceil( (_waveguide_delay_per_mm * _link_length) * _frequency + 
                                      _e_o_conversion_delay +
                                      _o_e_conversion_delay) );

   // Power Model
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model = new OpticalLinkPowerModel(_laser_modes, num_receivers_per_wavelength, link_frequency, waveguide_length, link_width);
}

OpticalLinkModel::~OpticalLinkModel()
{
   if (Config::getSingleton()->getEnablePowerModeling())
      delete _power_model;
}

void
OpticalLinkModel::processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   zero_load_delay += _net_link_delay;
  
   // Increment event counters
   UInt32 pkt_length = _model->getModeledLength(pkt);
   SInt32 num_flits = _model->computeNumFlits(pkt_length);

   if (num_endpoints == ENDPOINT_ALL)
   {
      assert(_laser_modes.broadcast);
      _total_link_broadcasts += num_flits;
   }
   else if (num_endpoints == 1)
   {
      if (_laser_modes.unicast)
         _total_link_unicasts += num_flits;
      else // (_laser_modes.broadcast) - only broadcast mode
         _total_link_broadcasts += num_flits;
   }
   else
      LOG_PRINT_ERROR("Unrecognized number of endpoints(%i)", num_endpoints);

   // Update dynamic energy counters
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->updateDynamicEnergy(num_flits, num_endpoints);
}
