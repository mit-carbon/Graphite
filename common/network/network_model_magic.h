#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network.h"
#include "core.h"
#include "lock.h"

class NetworkModelMagic : public NetworkModel
{
   private:
      bool _enabled;
     
      Lock _lock;

      UInt64 _num_packets;
      UInt64 _num_bytes;

   public:
      NetworkModelMagic(Network *net, SInt32 network_id, float network_frequency);
      ~NetworkModelMagic() { }

      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);

      void processReceivedPacket(NetPacket& pkt);

      void outputSummary(std::ostream &out);

      void enable()
      { _enabled = true; }

      void disable()
      { _enabled = false; }
};

#endif /* NETWORK_MODEL_MAGIC_H */
