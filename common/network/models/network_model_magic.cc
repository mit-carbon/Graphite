#include "network.h"
#include "network_model_magic.h"
#include "log.h"

NetworkModelMagic::NetworkModelMagic(Network *net, SInt32 network_id)
   : NetworkModel(net, network_id)
{
   _flit_width = -1;
   _has_broadcast_capability = false;
}

NetworkModelMagic::~NetworkModelMagic()
{}

void
NetworkModelMagic::routePacket(const NetPacket &pkt, queue<Hop> &next_hops)
{
   LOG_PRINT("Entering routePacket");
   // A latency of '1'
   Hop hop(pkt, TILE_ID(pkt.receiver), RECEIVE_TILE, Latency(1,_frequency), Latency(0,_frequency));
   next_hops.push(hop);
}
