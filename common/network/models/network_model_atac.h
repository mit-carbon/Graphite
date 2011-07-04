#pragma once

#include <vector>
#include <string>
using namespace std;

#include "queue_model.h"
#include "network.h"
#include "lock.h"
#include "electrical_network_router_model.h"
#include "electrical_network_link_model.h"
#include "optical_network_link_model.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtac : public NetworkModel
{
   private:
      enum NetworkComponentType
      {
         INVALID_COMPONENT = 0,
         MIN_COMPONENT_TYPE,
         SENDER_HUB = MIN_COMPONENT_TYPE,
         RECEIVER_HUB,
         SENDER_TILE,
         RECEIVER_TILE,
         MAX_COMPONENT_TYPE = RECEIVER_TILE,
         NUM_COMPONENT_TYPES = MAX_COMPONENT_TYPE - MIN_COMPONENT_TYPE
      };

      enum SubNetworkType
      {
         MESH_NETWORK = 0,
         OPTICAL_NETWORK,
         BROADCAST_NETWORK,
         NUM_SUB_NETWORK_TYPES
      };

      enum MeshNetworkOutputDirection
      {
         UP = 0,
         DOWN,
         LEFT,
         RIGHT,
         NUM_MESH_NETWORK_OUTPUT_DIRECTIONS,
         SELF
      };

      enum GlobalRoutingStrategy
      {
         CLUSTER_BASED = 0,
         DISTANCE_BASED
      };

      enum GlobalRoute
      {
         GLOBAL_ENET = 0,
         GLOBAL_ANET
      };

      enum LocalRoute
      {
         LOCAL_ENET = 0,
         LOCAL_BNET
      };

      tile_id_t m_tile_id;
      
      // ANet topology parameters
      static UInt32 m_total_tiles;
      static UInt32 m_num_clusters;
      static SInt32 m_cluster_size;
      static SInt32 m_sqrt_cluster_size;
      static SInt32 m_mesh_width;
      static SInt32 m_mesh_height;
      UInt32 m_num_broadcast_networks_per_cluster;
      
      // Routing Algorithm
      static GlobalRoutingStrategy m_global_routing_strategy;
      static LocalRoute m_local_route;
      static UInt32 m_unicast_routing_threshold;

      // Frequency Parameters
      volatile float m_mesh_network_frequency;
      volatile float m_optical_network_frequency;
      volatile float m_broadcast_network_frequency;

      // Latency Parameters
      UInt32 m_num_mesh_network_router_ports;
      UInt64 m_mesh_network_hop_delay;
      UInt64 m_optical_network_link_delay;
      UInt64 m_broadcast_network_delay;

      // Router & Link Power Models
      ElectricalNetworkRouterModel* m_mesh_network_router_model;
      ElectricalNetworkLinkModel* m_mesh_network_link_model;
      OpticalNetworkLinkModel* m_optical_network_link_model;
      ElectricalNetworkLinkModel* m_broadcast_network_link_model;

      // Link Width Parameters
      UInt32 m_mesh_network_link_width;
      UInt32 m_optical_network_link_width;
      UInt32 m_broadcast_network_link_width;

      // Link Type Parameters
      string m_mesh_network_link_type;
      string m_broadcast_network_link_type;

      // Bandwidth Parameters
      volatile double m_mesh_network_bandwidth;
      volatile double m_optical_network_bandwidth;
      volatile double m_broadcast_network_bandwidth;
      volatile double m_effective_anet_bandwidth;

      // Optical Hub models
      QueueModel* m_sender_hub_queue_model;
      QueueModel** m_receiver_hub_queue_models;
      // Locks for the Queue Models
      Lock m_sender_hub_lock;
      Lock m_receiver_hub_lock;

      // Gather Network Queue Models
      QueueModel* m_mesh_network_queue_models[NUM_MESH_NETWORK_OUTPUT_DIRECTIONS];

      // Queue Models
      bool m_queue_model_enabled;
      string m_queue_model_type;

      // General Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_sender_hub_contention_delay;
      UInt64 m_total_sender_hub_packets;
      UInt64 m_total_buffered_sender_hub_packets;

      UInt64* m_total_receiver_hub_contention_delay;
      UInt64* m_total_receiver_hub_packets;
      UInt64* m_total_buffered_receiver_hub_packets;

      // Event Counters
      UInt64 m_mesh_network_router_buffer_reads;
      UInt64 m_mesh_network_router_buffer_writes;
      UInt64 m_mesh_network_router_switch_allocator_traversals;
      UInt64 m_mesh_network_router_crossbar_traversals;
      UInt64 m_mesh_network_link_traversals;
      UInt64 m_optical_network_link_traversals;
      UInt64 m_broadcast_network_link_traversals;

      // Private Functions
      static SInt32 getClusterID(core_id_t core_id);
      static SInt32 getClusterID(tile_id_t tile_id);
      static tile_id_t getTileIDWithOpticalHub(SInt32 cluster_id);
      static void getTileIDListInCluster(SInt32 cluster_id, vector<tile_id_t>& tile_id_list);

      UInt64 routePacketOnMeshNetwork(const NetPacket& pkt, tile_id_t next_receiver);

      static UInt32 computeNumHopsOnMeshNetwork(tile_id_t sender, tile_id_t receiver);
      static void computePositionOnMeshNetwork(tile_id_t tile_id, SInt32& x, SInt32& y);
      static tile_id_t computeTileIdOnMeshNetwork(SInt32 x, SInt32 y);
      UInt64 computeLatencyOnMeshNetwork(MeshNetworkOutputDirection direction, UInt64 time, UInt32 pkt_length);
      tile_id_t getNextDestOnMeshNetwork(tile_id_t next_receiver, MeshNetworkOutputDirection& direction);

      UInt64 getHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id, UInt64 pkt_time, const NetPacket& pkt);

      static UInt64 computeProcessingTime(UInt32 pkt_length, volatile double bandwidth);

      static void initializeANetTopologyParams();
      void createANetRouterAndLinkModels();
      void destroyANetRouterAndLinkModels();
      
      void createOpticalHub();
      void destroyOpticalHub();
      void createQueueModels();
      void destroyQueueModels();

      void initializeEventCounters();

      void outputHubSummary(ostream& out);
      
      // Energy/Power related functions
      void updateDynamicEnergy(SubNetworkType sub_net_type, const NetPacket& pkt);
      void outputPowerSummary(ostream& out);

      // Routing
      GlobalRoute computeGlobalRoute(core_id_t sender, core_id_t receiver);
      static GlobalRoutingStrategy parseGlobalRoutingStrategy(string strategy);
      static LocalRoute parseLocalRoute(string route);

      UInt32 getFlitWidth() { return m_mesh_network_link_width; }

   public:
      NetworkModelAtac(Network *net, SInt32 network_id);
      ~NetworkModelAtac();

      volatile float getFrequency() { return m_mesh_network_frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      static pair<bool,SInt32> computeTileCountConstraints(SInt32 tile_count);
      static pair<bool, vector<tile_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count);
      static pair<bool, vector<vector<tile_id_t> > > computeProcessToTileMapping();
 
      void reset() {}
      void outputSummary(std::ostream &out);
      
      // Only for NetworkModelAtac
      UInt64 computeHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, const NetPacket& pkt);
      pair<tile_id_t,UInt64> __routePacketOnMeshNetwork(const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length, tile_id_t next_receiver);
};
