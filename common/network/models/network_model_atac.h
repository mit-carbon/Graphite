#pragma once

#include <vector>
#include <string>
using namespace std;

#include "queue_model.h"
#include "network.h"
#include "network_model.h"
#include "router_model.h"
#include "electrical_link_model.h"
#include "electrical_link_power_model.h"
#include "optical_link_model.h"
#include "optical_link_power_model.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtac : public NetworkModel
{
public:
   NetworkModelAtac(Network *net, SInt32 network_id);
   ~NetworkModelAtac();

   void routePacket(const NetPacket &pkt, queue<Hop> &nextHops);

   static bool isTileCountPermissible(SInt32 tile_count);
   static pair<bool, vector<tile_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count);
   static pair<bool, vector<vector<tile_id_t> > > computeProcessToTileMapping();

   void outputSummary(std::ostream &out, const Time& target_completion_time);

private:
   enum NodeType
   {
      EMESH = 2,  // Start at 2 always
      SEND_HUB,
      RECEIVE_HUB,
      STAR_NET_ROUTER_BASE
   };

   enum OutputDirection
   {
      SELF = 0,
      LEFT,
      RIGHT,
      DOWN,
      UP
   };

   enum GlobalRoutingStrategy
   {
      CLUSTER_BASED = 0,
      DISTANCE_BASED
   };

   enum GlobalRoute
   {
      GLOBAL_ENET = 0,
      GLOBAL_ONET
   };

   enum ReceiveNetType
   {
      BTREE = 0,
      STAR
   };

   static bool _initialized;
   
   // ENet
   static SInt32 _enet_width;
   static SInt32 _enet_height;
   
   // Clusters
   static SInt32 _num_clusters;
   static SInt32 _cluster_size;
   static SInt32 _numX_clusters;
   static SInt32 _numY_clusters;
   static SInt32 _cluster_width;
   static SInt32 _cluster_height;
   
   // Sub Clusters
   static SInt32 _num_access_points_per_cluster;
   static SInt32 _num_sub_clusters;
   static SInt32 _numX_sub_clusters;
   static SInt32 _numY_sub_clusters;
   static SInt32 _sub_cluster_width;
   static SInt32 _sub_cluster_height;
   
   // Cluster Boundaries and Access Points
   class ClusterInfo
   {
   public:
      class Boundary
      { 
      public:
         Boundary()
            : minX(0), maxX(0), minY(0), maxY(0) {}
         Boundary(SInt32 minX_, SInt32 maxX_, SInt32 minY_, SInt32 maxY_)
            : minX(minX_), maxX(maxX_), minY(minY_), maxY(maxY_) {}
         ~Boundary() {}
         SInt32 minX, maxX, minY, maxY;
      };
      Boundary _boundary;
      vector<tile_id_t> _access_point_list;
   };

   static vector<ClusterInfo> _cluster_info_list;
   
   // Type of Receive Network
   static ReceiveNetType _receive_net_type;
   // Num Receive Nets
   static SInt32 _num_receive_networks_per_cluster;
   
   // Global Routing Strategy
   static GlobalRoutingStrategy _global_routing_strategy;
   static SInt32 _unicast_distance_threshold;

   // Contention Modeling
   static bool _contention_model_enabled;

   // Injection Port Router
   RouterModel* _injection_router;

   // Latency Parameters
   SInt32 _num_enet_router_ports;
   // Router & Link Models
   // ENet Router & Link
   RouterModel* _enet_router;
   vector<ElectricalLinkModel*> _enet_link_list;
   
   // Send Hub Router
   RouterModel* _send_hub_router;
   // Receive Hub Router
   RouterModel* _receive_hub_router;
   // Optical Link
   OpticalLinkModel* _optical_link;
   
   // Htree Link List
   vector<ElectricalLinkModel*> _btree_link_list;
   // Star Net Router + Link List
   vector<RouterModel*> _star_net_router_list;
   vector<vector<ElectricalLinkModel*> > _star_net_link_list;

   // Private Functions
   void routePacketOnENet(const NetPacket& pkt, tile_id_t sender, tile_id_t receiver, queue<Hop>& next_hops);
   void routePacketOnONet(const NetPacket& pkt, tile_id_t sender, tile_id_t receiver, queue<Hop>& next_hops);

   static void initializeANetTopologyParams();
   void createANetRouterAndLinkModels();
   void destroyANetRouterAndLinkModels();
  
   // Output Summary 
   void outputPowerSummary(ostream& out, const Time& target_completion_time);
   void outputEventCountSummary(ostream& out);
   void outputContentionModelsSummary(ostream& out);
  
   // Static Functions
   static void initializeClusters();
   static void initializeAccessPointList(SInt32 cluster_id);
   static SInt32 getClusterID(tile_id_t tile_id);
   static SInt32 getSubClusterID(tile_id_t tile_id);
   static tile_id_t getNearestAccessPoint(tile_id_t tile_id);
   bool isAccessPoint(tile_id_t tile_id);
   static tile_id_t getTileIDWithOpticalHub(SInt32 cluster_id);
   static void getTileIDListInCluster(SInt32 cluster_id, vector<tile_id_t>& tile_id_list);
   static SInt32 getIndexInList(tile_id_t tile_id, vector<tile_id_t>& tile_id_list);
    
   static SInt32 computeNumHopsOnENet(tile_id_t sender, tile_id_t receiver);
   static void computePositionOnENet(tile_id_t tile_id, SInt32& x, SInt32& y);
   static tile_id_t computeTileIDOnENet(SInt32 x, SInt32 y);
   static SInt32 computeReceiveNetID(tile_id_t sender);

   // Compute Waveguide Length
   double computeOpticalLinkLength();

   // Routing
   static GlobalRoutingStrategy parseGlobalRoutingStrategy(string strategy);
   GlobalRoute computeGlobalRoute(tile_id_t sender, tile_id_t receiver);
   static ReceiveNetType parseReceiveNetType(string receive_net_type);
};
