#pragma once

#include <string>

#include "network_link_model.h"
#include "fixed_types.h"

class ElectricalNetworkLinkModel : public NetworkLinkModel
{
public:
   enum LinkType
   {
      ELECTRICAL_REPEATED = 0,
      ELECTRICAL_EQUALIZED,
      NUM_ELECTRICAL_LINK_TYPES
   };

   ElectricalNetworkLinkModel();
   virtual ~ElectricalNetworkLinkModel();

   static ElectricalNetworkLinkModel* create(std::string link_type_str, volatile float link_frequency, volatile double link_length, UInt32 link_width, bool use_orion = false);

private:
   static LinkType parseLinkType(std::string link_type_str);
};
