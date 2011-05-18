#include "network.h"
#include "network_model_magic.h"
#include "log.h"

NetworkModelMagic::NetworkModelMagic(Network *net, SInt32 network_id) : 
   NetworkModel(net, network_id)
{}

NetworkModelMagic::~NetworkModelMagic()
{}

UInt32
NetworkModelMagic::computeAction(const NetPacket& pkt)
{
   tile_id_t tile_id = getNetwork()->getTile()->getId();
   LOG_ASSERT_ERROR((pkt.receiver.tile_id == NetPacket::BROADCAST) || (pkt.receiver.tile_id == tile_id), \
         "pkt.receiver.tile_id(%i), tile_id(%i)", pkt.receiver.tile_id, tile_id);

   return RoutingAction::RECEIVE;
}

void
NetworkModelMagic::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   // A latency of '1'
   if (pkt.receiver.tile_id == NetPacket::BROADCAST)
   {
      UInt32 total_tiles = Config::getSingleton()->getTotalTiles();
   
      for (SInt32 i = 0; i < (SInt32) total_tiles; i++)
      {
         Hop h;
         h.final_dest.tile_id = NetPacket::BROADCAST;
         h.next_dest.tile_id = i;
         h.time = pkt.time + 1;

         nextHops.push_back(h);
      }
   }
   else
   {
      Hop h;
      h.final_dest.tile_id = pkt.receiver.tile_id;
      h.next_dest.tile_id = pkt.receiver.tile_id;
      h.time = pkt.time + 1;

      nextHops.push_back(h);
   }
}

void
NetworkModelMagic::processReceivedPacket(NetPacket &pkt)
{
   ScopedLock sl(_lock);

   // Update Receive Counters
   UInt64 latency = pkt.time - pkt.start_time;
   updateReceiveCounters(pkt, latency);
}

void NetworkModelMagic::outputSummary(std::ostream &out)
{
   NetworkModel::outputSummary(out);
}
