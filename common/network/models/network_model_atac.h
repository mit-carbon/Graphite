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

// #define TRACK_UTILIZATION     1

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
         GATHER_NETWORK = 0,
         OPTICAL_NETWORK,
         SCATTER_NETWORK,
         NUM_SUB_NETWORK_TYPES
      };

      tile_id_t m_tile_id;
      
      // ANet topology parameters
      static UInt32 m_total_tiles;
      static UInt32 m_num_clusters;
      static SInt32 m_cluster_size;
      static SInt32 m_sqrt_cluster_size;
      static SInt32 m_mesh_width;
      static SInt32 m_mesh_height;
      UInt32 m_num_scatter_networks_per_cluster;

      // Frequency Parameters
      volatile float m_gather_network_frequency;
      volatile float m_optical_network_frequency;
      volatile float m_scatter_network_frequency;

      // Latency Parameters
      UInt32 m_num_gather_network_router_ports;
      UInt32 m_num_hops_sender_tile_to_sender_hub;
      UInt64 m_gather_network_delay;
      UInt64 m_optical_network_link_delay;
      UInt64 m_scatter_network_delay;

      // Router & Link Power Models
      ElectricalNetworkRouterModel* m_gather_network_router_model;
      ElectricalNetworkLinkModel* m_gather_network_link_model;
      OpticalNetworkLinkModel* m_optical_network_link_model;
      ElectricalNetworkLinkModel* m_scatter_network_link_model;

      // Link Width Parameters
      UInt32 m_gather_network_link_width;
      UInt32 m_optical_network_link_width;
      UInt32 m_scatter_network_link_width;

      // Link Type Parameters
      string m_gather_network_link_type;
      string m_scatter_network_link_type;

      // Bandwidth Parameters
      volatile double m_gather_network_bandwidth;
      volatile double m_optical_network_bandwidth;
      volatile double m_scatter_network_bandwidth;
      volatile double m_effective_anet_bandwidth;

      // Optical Hub models
      QueueModel* m_sender_hub_queue_model;
      QueueModel** m_receiver_hub_queue_models;
      // Locks for the Queue Models
      Lock m_sender_hub_lock;
      Lock m_receiver_hub_lock;

      // Queue Models
      bool m_queue_model_enabled;
      string m_queue_model_type;
      bool m_enabled;

      // General Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_bytes_received;
      UInt64 m_total_packets_received;
      UInt64 m_total_contention_delay;
      UInt64 m_total_packet_latency;

      UInt64 m_total_sender_hub_contention_delay;
      UInt64 m_total_sender_hub_packets;
      UInt64 m_total_buffered_sender_hub_packets;

      UInt64* m_total_receiver_hub_contention_delay;
      UInt64* m_total_receiver_hub_packets;
      UInt64* m_total_buffered_receiver_hub_packets;

      // Activity Counters
      UInt64 m_gather_network_router_switch_allocator_traversals;
      UInt64 m_gather_network_router_crossbar_traversals;
      UInt64 m_gather_network_link_traversals;
      UInt64 m_optical_network_link_traversals;
      UInt64 m_scatter_network_link_traversals;

#ifdef TRACK_UTILIZATION
      vector<UInt64> m_sender_hub_utilization;
      vector<UInt64>* m_receiver_hub_utilization;
      UInt64 m_update_interval;
#endif

      // Private Functions
      static SInt32 getClusterID(core_id_t core_id);
      static SInt32 getClusterID(tile_id_t tile_id);
      static tile_id_t getTileIDWithOpticalHub(SInt32 cluster_id);
      static void getTileIDListInCluster(SInt32 cluster_id, vector<tile_id_t>& tile_id_list);

      UInt64 getHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id, UInt64 pkt_time, const NetPacket& pkt);

      UInt64 computeProcessingTime(UInt32 pkt_length, volatile double bandwidth);
      tile_id_t getRequester(const NetPacket& pkt);

      static void initializeANetTopologyParams();
      void createANetRouterAndLinkModels();
      void destroyANetRouterAndLinkModels();
      
      void createOpticalHub();
      void destroyOpticalHub();
      void createQueueModels();
      void destroyQueueModels();
      void resetQueueModels();

      void initializePerformanceCounters();
      void initializeActivityCounters();

      void outputHubSummary(ostream& out);
      
      // Energy/Power related functions
      void updateDynamicEnergy(SubNetworkType sub_net_type, const NetPacket& pkt);
      void outputPowerSummary(ostream& out);

#ifdef TRACK_UTILIZATION
      void initializeUtilizationCounters();
      void destroyUtilizationCounters();
      void updateUtilization(NetworkComponentType hub_type, SInt32 hub_id, UInt64 pkt_time, UInt64 processing_time);
      void updateVector(UInt64 interval_id, vector<UInt64>& utilization_vec);
      void outputUtilizationSummary();
#endif

   public:
      NetworkModelAtac(Network *net, SInt32 network_id);
      ~NetworkModelAtac();

      volatile float getFrequency() { return m_gather_network_frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      static pair<bool,SInt32> computeTileCountConstraints(SInt32 tile_count);
      static pair<bool, vector<tile_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count);
      static pair<bool, vector<vector<tile_id_t> > > computeProcessToTileMapping();
 
      void enable();
      void disable();
      void reset();
      void outputSummary(std::ostream &out);
      
      // Only for NetworkModelAtac
      UInt64 computeHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, const NetPacket& pkt);
};
