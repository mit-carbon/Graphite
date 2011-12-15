#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network.h"
#include "tile.h"
#include "lock.h"

class NetworkModelMagic : public NetworkModel
{
   public:
      NetworkModelMagic(Network *net, SInt32 network_id);
      ~NetworkModelMagic();

      volatile float getFrequency() { return 1.0; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      void outputSummary(std::ostream &out);

   private:
      Lock _lock;

      UInt32 getFlitWidth() { return 1; }
};

#endif /* NETWORK_MODEL_MAGIC_H */
