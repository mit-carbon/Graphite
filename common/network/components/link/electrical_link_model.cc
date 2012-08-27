#include <cmath>
#include "electrical_link_model.h"
#include "network_model.h"
#include "network.h"
#include "log.h"

ElectricalLinkModel::ElectricalLinkModel(NetworkModel* model, string link_type,
                                         float link_frequency, double link_length, UInt32 link_width)
   : LinkModel(model, link_frequency, link_length, link_width)
   , _power_model(NULL)
   , _total_link_traversals(0)
{
   // Link Delay
   // Approximately, propagation speed = 10 ps/mm = 0.01 ns/mm
   float propagation_speed = 0.01;
   _net_link_delay = ceil(link_frequency * propagation_speed * link_length);

   // Power model
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model = new ElectricalLinkPowerModel(link_type, link_frequency, link_length, link_width);
}

ElectricalLinkModel::~ElectricalLinkModel()
{
   if (Config::getSingleton()->getEnablePowerModeling())
      delete _power_model;
}

void
ElectricalLinkModel::processPacket(const NetPacket& pkt, UInt64& zero_load_delay)
{
   if (!_model->isModelEnabled(pkt))
      return;

   zero_load_delay += _net_link_delay;

   // Update event counters
   UInt32 pkt_length = _model->getModeledLength(pkt);
   SInt32 num_flits = _model->computeNumFlits(pkt_length);
   _total_link_traversals += num_flits;
   
   // Update dynamic energy
   if (Config::getSingleton()->getEnablePowerModeling())
      _power_model->updateDynamicEnergy(num_flits);
}
