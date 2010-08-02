#include "network_link_model.h"
#include "electrical_network_repeated_link_model.h"
#include "electrical_network_equalized_link_model.h"
#include "optical_network_link_model.h"
#include "network_link_model_orion.h"
#include "log.h"

NetworkLinkModel::NetworkLinkModel()
{}

NetworkLinkModel::~NetworkLinkModel()
{}

NetworkLinkModel*
NetworkLinkModel::create(std::string link_type_str, volatile float link_frequency, volatile double link_length, UInt32 link_width, bool use_orion)
{
   LinkType link_type = parseLinkType(link_type_str);

   if (! use_orion)
   {
      switch (link_type)
      {
         case ELECTRICAL_REPEATED:
            return new ElectricalNetworkRepeatedLinkModel(link_frequency, link_length, link_width);

         case ELECTRICAL_EQUALIZED:
            return new ElectricalNetworkEqualizedLinkModel(link_frequency, link_length, link_width);

         case OPTICAL:
            return new OpticalNetworkLinkModel(link_frequency, link_length, link_width);

         default:
            LOG_PRINT_ERROR("Unrecognized Link Type(%u)", link_type);
            return (NetworkLinkModel*) NULL;
      }
   }
   else
   {
      return new NetworkLinkModelOrion(link_type, link_frequency, link_length, link_width);
   }
}

NetworkLinkModel::LinkType
NetworkLinkModel::parseLinkType(std::string link_type_str)
{
   if (link_type_str == "electrical_repeated")
      return ELECTRICAL_REPEATED;
   else if (link_type_str == "electrical_equalized")
      return ELECTRICAL_EQUALIZED;
   else if (link_type_str == "optical")
      return OPTICAL;
   else
   {
      LOG_PRINT_ERROR("Unrecognized Link Type(%s)", link_type_str.c_str());
      return NUM_LINK_TYPES;
   }
}
