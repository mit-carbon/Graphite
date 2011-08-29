#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network_model.h"

class NetworkModelMagic : public NetworkModel
{
public:
   NetworkModelMagic(Network *net, SInt32 network_id);
   ~NetworkModelMagic();

   void routePacket(const NetPacket &pkt, queue<Hop>& next_hops);
   void outputSummary(std::ostream &out);

   void reset() {}
};

#endif /* NETWORK_MODEL_MAGIC_H */
