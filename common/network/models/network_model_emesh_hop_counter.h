#ifndef NETWORK_MODEL_EMESH_HOP_COUNTER_H
#define NETWORK_MODEL_EMESH_HOP_COUNTER_H

#include "network.h"
#include "network_model.h"
#include "lock.h"

class NetworkModelEMeshHopCounter : public NetworkModel
{
public:
   NetworkModelEMeshHopCounter(Network *net, SInt32 network_id);
   ~NetworkModelEMeshHopCounter();

   void routePacket(const NetPacket &pkt, queue<Hop> &next_hops);
   void outputSummary(std::ostream &out);

   void reset() {}

private:
   SInt32 _mesh_width;
   SInt32 _mesh_height;

   // Private Functions
   void computePosition(tile_id_t tile, SInt32 &x, SInt32 &y);
   SInt32 computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2);
};

#endif
