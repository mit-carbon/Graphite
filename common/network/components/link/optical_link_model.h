#pragma once

#include <string>
using std::string;

#include "link_model.h"
#include "fixed_types.h"

class NetPacket;
class NetworkModel;
class OpticalLinkPowerModel;

class OpticalLinkModel : public LinkModel
{
public:
   class LaserModes
   {
   public:
      LaserModes(): unicast(false), broadcast(false) {}
      LaserModes(const LaserModes& modes): unicast(modes.unicast), broadcast(modes.broadcast) {}
      ~LaserModes() {}
      bool unicast;
      bool broadcast;
   };

   enum LaserType
   {
      STANDARD = 0,
      THROTTLED
   };

   OpticalLinkModel(NetworkModel* model, UInt32 num_readers_per_wavelength,
                    double frequency, double voltage, double waveguide_length, UInt32 link_width);
   ~OpticalLinkModel();

   // processPacket() called at every link
   void processPacket(const NetPacket& pkt, SInt32 num_endpoints, UInt64& zero_load_delay);
   static const SInt32 ENDPOINT_ALL = -1;
  
   // Get laser modes, type
   LaserModes getLaserModes() const   { return _laser_modes; }

   // Event counters specific to OpticalLink
   UInt64 getTotalUnicasts() const    { return _total_link_unicasts; }
   UInt64 getTotalBroadcasts() const  { return _total_link_broadcasts; }

   // Energy Models
   OpticalLinkPowerModel* getPowerModel() { return _power_model; }

private:
   // Power model
   OpticalLinkPowerModel* _power_model;

   // Number of readers on a wavelength
   UInt32 _num_readers_per_wavelength;

   // Possible laser modes - (unicast, broadcast)
   LaserModes _laser_modes;
   // Laser type - (standard, throttled)
   LaserType _laser_type;
   // Thermal tuning strategy - (full_thermal, thermal_reshuffle, electrical_assist, athermal)
   string _ring_tuning_strategy;

   // Event Counters
   UInt64 _total_link_unicasts;
   UInt64 _total_link_broadcasts;

   // Parse laser modes
   static LaserModes parseLaserModes(string laser_modes_str);
   static LaserType parseLaserType(string laser_type_str);
};
