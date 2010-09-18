#include "electrical_network_link_model.h"
#include "electrical_network_repeated_link_model.h"
#include "electrical_network_equalized_link_model.h"
#include "electrical_network_link_model_orion.h"
#include "log.h"

ElectricalNetworkLinkModel::ElectricalNetworkLinkModel()
{}

ElectricalNetworkLinkModel::~ElectricalNetworkLinkModel()
{}

ElectricalNetworkLinkModel*
ElectricalNetworkLinkModel::create(std::string link_type_str, volatile float link_frequency, volatile double link_length, UInt32 link_width, bool use_orion)
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

         default:
            LOG_PRINT_ERROR("Unrecognized Link Type(%u)", link_type);
            return (ElectricalNetworkLinkModel*) NULL;
      }
   }
   else
   {
      return new ElectricalNetworkLinkModelOrion(link_type, link_frequency, link_length, link_width);
   }
}

ElectricalNetworkLinkModel::LinkType
ElectricalNetworkLinkModel::parseLinkType(std::string link_type_str)
{
   if (link_type_str == "electrical_repeated")
      return ELECTRICAL_REPEATED;
   else if (link_type_str == "electrical_equalized")
      return ELECTRICAL_EQUALIZED;
   else
   {
      LOG_PRINT_ERROR("Unrecognized Link Type(%s)", link_type_str.c_str());
      return NUM_ELECTRICAL_LINK_TYPES;
   }
}
