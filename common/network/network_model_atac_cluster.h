#ifndef __NETWORK_MODEL_ATAC_CLUSTER_H__
#define __NETWORK_MODEL_ATAC_CLUSTER_H__

#include <string>
using namespace std;

#include "queue_model.h"
#include "network.h"
#include "lock.h"
#include "network_link_model.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtacCluster : public NetworkModel
{
   private:
      enum NetworkComponentType
      {
         INVALID_COMPONENT = 0,
         MIN_COMPONENT_TYPE,
         SENDER_HUB = MIN_COMPONENT_TYPE,
         RECEIVER_HUB,
         SENDER_CORE,
         RECEIVER_CORE,
         MAX_COMPONENT_TYPE = RECEIVER_CORE,
         NUM_COMPONENT_TYPES = MAX_COMPONENT_TYPE - MIN_COMPONENT_TYPE
      };

      enum SubNetworkType
      {
         GATHER_NETWORK = 0,
         OPTICAL_NETWORK,
         SCATTER_NETWORK,
         NUM_SUB_NETWORK_TYPES
      };

      core_id_t m_core_id;
      
      // ANet topology parameters
      UInt32 m_total_cores;
      UInt32 m_num_clusters;
      UInt32 m_num_scatter_networks_per_cluster;
      static SInt32 m_cluster_size;
      static SInt32 m_sqrt_cluster_size;
      static SInt32 m_mesh_width;
      static SInt32 m_mesh_height;

      // Frequency Parameters
      volatile float m_gather_network_frequency;
      volatile float m_optical_network_frequency;
      volatile float m_scatter_network_frequency;

      // Latency Parameters
      UInt32 m_num_hops_sender_core_to_sender_hub;
      UInt64 m_gather_network_link_delay;
      UInt64 m_gather_network_delay;

      UInt64 m_optical_network_link_delay;
      UInt64 m_scatter_network_delay;

      // Link Power Models
      NetworkLinkModel* m_gather_network_link_model;
      NetworkLinkModel* m_optical_network_link_model;
      NetworkLinkModel* m_scatter_network_link_model;

      // Link Width Parameters
      UInt32 m_gather_network_link_width;
      UInt32 m_optical_network_link_width;
      UInt32 m_scatter_network_link_width;

      // Link Type Parameters
      string m_gather_network_link_type;
      string m_optical_network_link_type;
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
      UInt64 m_total_packet_latency;

      UInt64 m_total_sender_hub_contention_delay;
      UInt64 m_total_sender_hub_packets;

      UInt64* m_total_receiver_hub_contention_delay;
      UInt64* m_total_receiver_hub_packets;

      // Private Functions
      static SInt32 getClusterID(core_id_t core_id);
      static core_id_t getCoreIDWithOpticalHub(SInt32 cluster_id);
      static void getCoreIDListInCluster(SInt32 cluster_id, vector<core_id_t>& core_id_list);

      UInt64 getHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id, UInt64 pkt_time, UInt32 pkt_length, PacketType pkt_type, core_id_t requester);

      UInt64 computeProcessingTime(UInt32 pkt_length, volatile double bandwidth);
      core_id_t getRequester(const NetPacket& pkt);

      void initializeANetTopologyParams();
      void createANetLinkModels();
      void createOpticalHub();
      void initializePerformanceCounters();

      void outputHubSummary(ostream& out);
      
      void destroyOpticalHub();
      void destroyANetLinkModels();

      // Energy/Power related functions
      void updateDynamicEnergy(SubNetworkType sub_net_type, const NetPacket& pkt);
      void outputPowerSummary(ostream& out);

   public:
      NetworkModelAtacCluster(Network *net, SInt32 network_id);
      ~NetworkModelAtacCluster();

      volatile float getFrequency() { return m_gather_network_frequency; }

      UInt32 computeAction(const NetPacket& pkt);
      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);

      static pair<bool, vector<core_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count);
 
      void enable();
      void disable();
      void outputSummary(std::ostream &out);
      
      // Only for NetworkModelAtacCluster
      UInt64 computeHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, UInt32 pkt_length, core_id_t requester);
};

#endif /* __NETWORK_MODEL_ATAC_CLUSTER_H__ */
