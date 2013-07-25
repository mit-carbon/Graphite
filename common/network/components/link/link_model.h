#pragma once

#include "fixed_types.h"

class NetworkModel;

class LinkModel
{
public:
   LinkModel(NetworkModel* model, float frequency, double link_length, UInt32 link_width)
      : _model(model)
      , _frequency(frequency)
      , _link_length(link_length)
      , _link_width(link_width)
      , _net_link_delay(0)
   {}
   virtual ~LinkModel() {}

   UInt64 getDelay() { return _net_link_delay; }

protected:
   // Input parameters
   NetworkModel* _model;
   float _frequency;
   double _link_length;
   UInt32 _link_width;
  
   // Output parameters 
   UInt64 _net_link_delay;
};
