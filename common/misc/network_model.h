#ifndef NETWORK_MODEL_H
#define NETWORK_MODEL_H

class NetPacket;
class Network;

#include <string>

// -- Network Models -- //

// To implement a new network model, you must implement this routing
// object. To route, take a packet and compute the next hop(s) and the
// time stamp for when that packet will be forwarded.
//   This lets one implement "magic" networks, analytical models,
// realistic hop-by-hop modeling, as well as broadcast models, such as
// a bus or ATAC.  Each static network has its own model object. This
// lets the user network be modeled accurately, while the MCP is a
// stupid magic network.
//   A packet will be dropped if no hops are filled in the nextHops
// vector.
class NetworkModel
{
   public:
      NetworkModel(Network *network) : _network(network) { }
      virtual ~NetworkModel() { }

      struct Hop
      {
         SInt32 dest;
         UInt64 time;
      };

      virtual void routePacket(const NetPacket &pkt,
                               std::vector<Hop> &nextHops) = 0;

      virtual void outputSummary(std::ostream &out) = 0;

      static NetworkModel *createModel(Network *network, UInt32 type);

      static UInt32 parseNetworkType(std::string str);

   protected:
      Network *getNetwork() { return _network; }

   private:
      Network *_network;

};

#endif // NETWORK_MODEL_H
