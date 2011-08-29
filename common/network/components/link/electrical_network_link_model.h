#pragma once

#include <string>

#include "network_link_model.h"
#include "fixed_types.h"

class NetPacket;
class NetworkModel;

class ElectricalNetworkLinkModel : public NetworkLinkModel
{
public:
   enum LinkType
   {
      ELECTRICAL_REPEATED = 0,
      ELECTRICAL_EQUALIZED,
      NUM_ELECTRICAL_LINK_TYPES
   };

   ElectricalNetworkLinkModel(NetworkModel* model, volatile float frequency,
                              volatile double link_length, UInt32 link_width);
   virtual ~ElectricalNetworkLinkModel();

   void processPacket(const NetPacket& pkt, UInt64& zero_load_delay);
   UInt64 getTotalTraversals() { return _total_link_traversals; }

   static ElectricalNetworkLinkModel* create(std::string link_type_str, NetworkModel* model, 
         volatile float link_frequency, volatile double link_length, UInt32 link_width);

private:
   static LinkType parseLinkType(std::string link_type_str);

   // Event Counters
   UInt64 _total_link_traversals;
};
