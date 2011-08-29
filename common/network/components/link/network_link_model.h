#pragma once

#include <string>

#include "fixed_types.h"

class NetworkModel;

class NetworkLinkModel
{
public:
   NetworkLinkModel(NetworkModel* model, volatile float frequency,
                    volatile double link_length, UInt32 link_width)
      : _model(model)
      , _frequency(frequency)
      , _link_length(link_length)
      , _link_width(link_width)
      , _total_dynamic_energy(0.0)
   {}
   virtual ~NetworkLinkModel() {}

   UInt64 getDelay() { return _net_link_delay; }
   virtual volatile double getStaticPower() { return _total_static_power; }
   virtual void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1) = 0;
   volatile double getDynamicEnergy() { return _total_dynamic_energy; }

protected:
   NetworkModel* _model;
   volatile float _frequency;
   volatile double _link_length;
   UInt32 _link_width;

   // Total Link Delay
   UInt64 _net_link_delay;
   // Total Static Power
   volatile double _total_static_power;
   // Total Dynamic Energy
   volatile double _total_dynamic_energy;
};
