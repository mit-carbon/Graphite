#ifndef NETWORK_MODEL_EMESH_HOP_COUNTER_H
#define NETWORK_MODEL_EMESH_HOP_COUNTER_H

#include "network.h"
#include "network_model.h"
#include "lock.h"
#include "electrical_network_link_model.h"
#include "electrical_network_router_model.h"

class NetworkModelEMeshHopCounter : public NetworkModel
{
public:
   NetworkModelEMeshHopCounter(Network *net, SInt32 network_id);
   ~NetworkModelEMeshHopCounter();

   volatile float getFrequency() { return _frequency; }
   
   UInt32 computeAction(const NetPacket& pkt);
   void routePacket(const NetPacket &pkt,
                    std::vector<Hop> &nextHops);
   void processReceivedPacket(NetPacket &pkt);

   void outputSummary(std::ostream &out);

   void reset();

private:

   volatile float _frequency;

   // Topology Parameters
   UInt64 _hop_latency;
   UInt32 _num_router_ports;
   UInt32 _link_width;
   std::string _link_type;
   
   SInt32 _mesh_width;
   SInt32 _mesh_height;

   static UInt32 _NUM_OUTPUT_DIRECTIONS;

   Lock _lock;

   // Router & Link Models
   ElectricalNetworkRouterModel* _electrical_router_model;
   ElectricalNetworkLinkModel* _electrical_link_model;

   // Activity Counters
   UInt64 _switch_allocator_traversals;
   UInt64 _crossbar_traversals;
   UInt64 _link_traversals;

   // Private Functions
   void computePosition(tile_id_t tile, SInt32 &x, SInt32 &y);
   SInt32 computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2);

   UInt64 computeProcessingTime(UInt32 pkt_length);

   void initializePerformanceCounters();
   void initializeActivityCounters();

   // Power/Energy related
   void createRouterAndLinkModels();
   void destroyRouterAndLinkModels();

   void updateDynamicEnergy(const NetPacket& pkt, UInt32 contention, UInt32 num_hops);
   void outputPowerSummary(std::ostream& out); 
};

#endif
