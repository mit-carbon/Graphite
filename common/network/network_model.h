#ifndef NETWORK_MODEL_H
#define NETWORK_MODEL_H

class NetPacket;
class Network;

#include <string>
#include <vector>

#include "config.h"
#include "packet_type.h"
#include "fixed_types.h"

#define CORE_ID(x)         ((core_id_t) {x, MAIN_CORE_TYPE})
#define TILE_ID(x)         (x.tile_id)

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
         core_id_t final_dest;
         core_id_t next_dest;

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

      void enable() { _enabled = true; }
      void disable() { _enabled = false; }
      virtual void reset() = 0;

      // Update Send & Receive Counters
      void updateSendCounters(const NetPacket& packet);

      static NetworkModel *createModel(Network* network, SInt32 network_id, UInt32 model_type);
      static UInt32 parseNetworkType(std::string str);

      static std::pair<bool,SInt32> computeTileCountConstraints(UInt32 network_type, SInt32 tile_count);
      static std::pair<bool, std::vector<tile_id_t> > computeMemoryControllerPositions(UInt32 network_type, SInt32 num_memory_controllers, SInt32 total_tiles);
      static std::pair<bool, std::vector<Config::TileList> > computeProcessToTileMapping(UInt32 network_type);

   protected:
      Network *getNetwork() { return _network; }
      SInt32 getNetworkId() { return _network_id; }
      bool isEnabled() { return _enabled; }

      // Get Requester of a Packet
      tile_id_t getRequester(const NetPacket& packet);

      // Update Receive Counters
      void updateReceiveCounters(const NetPacket& packet, UInt64 zero_load_latency);

   private:
      Network *_network;
      
      SInt32 _network_id;
      std::string _network_name;
      bool _enabled;

      // Event Counters
      UInt64 _total_packets_sent;
      UInt64 _total_flits_sent;
      UInt64 _total_bytes_sent;

      UInt64 _total_packets_broadcasted;
      UInt64 _total_flits_broadcasted;
      UInt64 _total_bytes_broadcasted;

      UInt64 _total_packets_received;
      UInt64 _total_flits_received;
      UInt64 _total_bytes_received;

      UInt64 _total_packet_latency;
      UInt64 _total_contention_delay;

      // Initialize Event Counters
      void initializeEventCounters();
      // Compute Number of Flits
      UInt32 computeNumFlits(UInt32 pkt_length);
      // Get Flit Width
      virtual UInt32 getFlitWidth() = 0;
};

#endif // NETWORK_MODEL_H
