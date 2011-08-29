#include "electrical_network_link_model.h"
#include "electrical_network_repeated_link_model.h"
#include "electrical_network_equalized_link_model.h"
#include "network_model.h"
#include "network.h"
#include "log.h"

ElectricalNetworkLinkModel::ElectricalNetworkLinkModel(NetworkModel* model, volatile float frequency,
                                                       volatile double link_length, UInt32 link_width)
   : NetworkLinkModel(model, frequency, link_length, link_width)
   , _total_link_traversals(0)
{}

ElectricalNetworkLinkModel::~ElectricalNetworkLinkModel()
{}

ElectricalNetworkLinkModel*
ElectricalNetworkLinkModel::create(std::string link_type_str, NetworkModel* model, volatile float link_frequency,
                                   volatile double link_length, UInt32 link_width)
{
   LinkType link_type = parseLinkType(link_type_str);

   switch (link_type)
   {
      case ELECTRICAL_REPEATED:
         return new ElectricalNetworkRepeatedLinkModel(model, link_frequency, link_length, link_width);

      case ELECTRICAL_EQUALIZED:
         return new ElectricalNetworkEqualizedLinkModel(model, link_frequency, link_length, link_width);

      default:
         LOG_PRINT_ERROR("Unrecognized Link Type(%u)", link_type);
         return (ElectricalNetworkLinkModel*) NULL;
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

void
ElectricalNetworkLinkModel::processPacket(const NetPacket& pkt, UInt64& zero_load_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   zero_load_delay += _net_link_delay;

   // Event Counters
   UInt32 pkt_length = _model->getModeledLength(pkt);
   SInt32 num_flits = _model->computeNumFlits(pkt_length);
   _total_link_traversals += num_flits;
}
