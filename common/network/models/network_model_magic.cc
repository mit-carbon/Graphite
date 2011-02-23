#include "network.h"
#include "network_model_magic.h"
#include "memory_manager_base.h"
#include "log.h"

NetworkModelMagic::NetworkModelMagic(Network *net, SInt32 network_id) : 
   NetworkModel(net, network_id),
   _enabled(false),
   _num_packets(0),
   _num_bytes(0)
{ }

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

   tile_id_t requester = INVALID_TILE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getTile()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender.tile_id;
   
   LOG_ASSERT_ERROR((requester >= 0) && (requester < (tile_id_t) Config::getSingleton()->getTotalTiles()),
         "requester(%i)", requester);

   if ( (!_enabled) || (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) )
      return;

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   _num_packets ++;
   _num_bytes += pkt_length;
}

void NetworkModelMagic::outputSummary(std::ostream &out)
{
   out << "    num packets received: " << _num_packets << std::endl;
   out << "    num bytes received: " << _num_bytes << std::endl;
}
