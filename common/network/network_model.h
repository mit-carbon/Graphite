#ifndef NETWORK_MODEL_H
#define NETWORK_MODEL_H

class NetPacket;
class Network;

#include <string>
#include <vector>

#include "packet_type.h"
#include "fixed_types.h"

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
      NetworkModel(Network *network, SInt32 network_id);
      virtual ~NetworkModel() { }

      class Hop
      {
      public:
         Hop(): 
            final_dest(INVALID_CORE_ID), 
            next_dest(INVALID_CORE_ID), 
            specific(0), 
            time(0) 
         {}
         ~Hop() {}

         // Final & Next destinations of a packet
         // 'final_dest' field is used to fill in the 'receiver' field in NetPacket
         SInt32 final_dest;
         SInt32 next_dest;

         // This field may be used by network models to fill in the 'specific' field in NetPacket
         // In general, specific field can be used by different network models for different functions
         UInt32 specific;
         
         // This field fills in the 'time' field in NetPacket
         UInt64 time;
      };

      class RoutingAction
      {
      public:
         enum type_t
         {
            RECEIVE = 0x001,
            FORWARD = 0x010,
            DROP = 0x100
         };
      };

      virtual volatile float getFrequency() = 0;

      virtual UInt32 computeAction(const NetPacket& pkt) = 0;
      virtual void routePacket(const NetPacket &pkt,
                               std::vector<Hop> &nextHops) = 0;
      virtual void processReceivedPacket(NetPacket &pkt) = 0;

      virtual void outputSummary(std::ostream &out) = 0;

      virtual void enable() = 0;
      virtual void disable() = 0;

      static NetworkModel *createModel(Network* network, SInt32 network_id, UInt32 model_type);
      static UInt32 parseNetworkType(std::string str);

      static std::pair<bool,SInt32> computeCoreCountConstraints(UInt32 network_type, SInt32 core_count);
      static std::pair<bool, std::vector<core_id_t> > computeMemoryControllerPositions(UInt32 network_type, SInt32 num_memory_controllers, SInt32 total_cores);

   protected:
      Network *getNetwork() { return _network; }

   private:
      Network *_network;
      
      SInt32 _network_id;
      std::string _network_name;
};

#endif // NETWORK_MODEL_H
