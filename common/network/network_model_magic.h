#ifndef NETWORK_MODEL_MAGIC_H
#define NETWORK_MODEL_MAGIC_H

#include "network.h"
#include "core.h"

class NetworkModelMagic : public NetworkModel
{
   private:
      UInt64 _bytesSent;

   public:
      NetworkModelMagic(Network *net) : NetworkModel(net), _bytesSent(0) { }
      ~NetworkModelMagic() { }

      void routePacket(const NetPacket &pkt,
                       std::vector<Hop> &nextHops)
      {
         Hop h;
         h.dest = pkt.receiver;
         h.time = getNetwork()->getCore()->getPerformanceModel()->getCycleCount();
         nextHops.push_back(h);

         _bytesSent += pkt.length;
      }

      void outputSummary(std::ostream &out)
      {
         out << "    bytes sent: " << _bytesSent << std::endl;
      }
};

#endif
