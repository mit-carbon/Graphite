#pragma once

#include "fixed_types.h"

class NetworkModel;

class LinkModel
{
public:
   LinkModel(NetworkModel* model)
      : _model(model)
      , _net_link_delay(0)
   {}
   virtual ~LinkModel() {}

   UInt64 getDelay() { return _net_link_delay; }

protected:
   // Input parameters
   NetworkModel* _model;
  
   // Output parameters 
   UInt64 _net_link_delay;
};
