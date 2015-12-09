#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network_model.h"

class NetworkModelMagic : public NetworkModel
{
public:
   NetworkModelMagic(Network *net, SInt32 network_id);
   ~NetworkModelMagic();

   void routePacket(NetPacket &pkt, queue<Hop>& next_hops);
};

#endif /* NETWORK_MODEL_MAGIC_H */
