#ifndef NETWORK_MODEL_HOP_COUNTER_H
#define NETWORK_MODEL_HOP_COUNTER_H

#include "network.h"
#include "network_model.h"
#include "lock.h"

class NetworkModelHopCounter : public NetworkModel
{
public:
   NetworkModelHopCounter(Network *net);
   ~NetworkModelHopCounter();

   void routePacket(const NetPacket &pkt,
                    std::vector<Hop> &nextHops);

   void outputSummary(std::ostream &out);

   void enable() { _enabled = true; }
   void disable() { _enabled = false; }

private:

   void computePosition(core_id_t core, SInt32 &x, SInt32 &y);
   SInt32 computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2);

   bool _enabled;

   Lock _lock;
   UInt64 _bytesSent;
   UInt64 _cyclesLatency;
   UInt64 _hopLatency;

   SInt32 _meshWidth;
   SInt32 _meshHeight;
};

#endif
