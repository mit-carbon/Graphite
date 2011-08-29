#include <stdlib.h>
#include <math.h>

#include "network_model_emesh_hop_counter.h"
#include "simulator.h"
#include "config.h"
#include "config.h"
#include "tile.h"

NetworkModelEMeshHopCounter::NetworkModelEMeshHopCounter(Network *net, SInt32 network_id)
   : NetworkModel(net, network_id)
{
   SInt32 total_tiles = Config::getSingleton()->getTotalTiles();

   _mesh_width = (SInt32) floor (sqrt(total_tiles));
   _mesh_height = (SInt32) ceil (1.0 * total_tiles / _mesh_width);

   try
   {
      _frequency = Sim()->getCfg()->getFloat("network/emesh_hop_counter/frequency");
      _flit_width = Sim()->getCfg()->getInt("network/emesh_hop_counter/flit_width");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_counter paramters from the cfg file");
   }

   // Broadcast Capability
   _has_broadcast_capability = false;

   assert(total_tiles <= _mesh_width * _mesh_height);
   assert(total_tiles > (_mesh_width - 1) * _mesh_height);
   assert(total_tiles > _mesh_width * (_mesh_height - 1));
}

NetworkModelEMeshHopCounter::~NetworkModelEMeshHopCounter()
{}

void
NetworkModelEMeshHopCounter::computePosition(tile_id_t tile,
                                             SInt32 &x, SInt32 &y)
{
   x = tile % _mesh_width;
   y = tile / _mesh_width;
}

SInt32
NetworkModelEMeshHopCounter::computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2)
{
   return abs(x1 - x2) + abs(y1 - y2);
}

void
NetworkModelEMeshHopCounter::routePacket(const NetPacket &pkt, queue<Hop> &next_hops)
{
   SInt32 sx, sy, dx, dy;

   computePosition(TILE_ID(pkt.sender), sx, sy);
   computePosition(TILE_ID(pkt.receiver), dx, dy);

   UInt32 num_hops = computeDistance(sx, sy, dx, dy);
   UInt64 latency = num_hops * 2;   // 1 - router, 1 -link

   Hop hop(pkt, TILE_ID(pkt.receiver), RECEIVE_TILE, latency, 0);
   next_hops.push(hop);
}

void
NetworkModelEMeshHopCounter::outputSummary(ostream& out)
{
   NetworkModel::outputSummary(out);
}
