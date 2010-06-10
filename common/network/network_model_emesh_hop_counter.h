#ifndef NETWORK_MODEL_EMESH_HOP_COUNTER_H
#define NETWORK_MODEL_EMESH_HOP_COUNTER_H

#include "network.h"
#include "network_model.h"
#include "lock.h"

class NetworkModelEMeshHopCounter : public NetworkModel
{
public:
   NetworkModelEMeshHopCounter(Network *net, SInt32 network_id, float network_frequency);
   ~NetworkModelEMeshHopCounter();

   void routePacket(const NetPacket &pkt,
                    std::vector<Hop> &nextHops);
   void processReceivedPacket(NetPacket &pkt);

   void outputSummary(std::ostream &out);

   void enable() { _enabled = true; }
   void disable() { _enabled = false; }

private:

   void computePosition(core_id_t core, SInt32 &x, SInt32 &y);
   SInt32 computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2);

   UInt64 computeSerializationLatency(UInt32 pkt_length);

   UInt64 _hopLatency;
   UInt32 _linkBandwidth;
   
   SInt32 _meshWidth;
   SInt32 _meshHeight;

   bool _enabled;

   Lock _lock;
   
   // Performance Counters
   UInt64 _num_packets;
   UInt64 _num_bytes;
   UInt64 _total_latency;
};

#endif
