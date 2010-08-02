#pragma once

#include <string>

#include "fixed_types.h"

class NetworkLinkModel
{
public:
   enum LinkType
   {
      ELECTRICAL_REPEATED = 0,
      ELECTRICAL_EQUALIZED,
      OPTICAL,
      NUM_LINK_TYPES
   };

   NetworkLinkModel();
   virtual ~NetworkLinkModel();

   static NetworkLinkModel* create(std::string link_type_str, volatile float link_frequency, volatile double link_length, UInt32 link_width, bool use_orion = false);

   virtual UInt64 getDelay() = 0;
   virtual volatile double getStaticPower() = 0;
   virtual void updateDynamicEnergy(UInt32 num_bit_flips, UInt32 num_flits = 1) = 0;
   virtual volatile double getDynamicEnergy() = 0;
   
private:
   static LinkType parseLinkType(std::string link_type_str);
};
