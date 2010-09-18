#ifndef __NETWORK_MODEL_ATAC_CLUSTER_H__
#define __NETWORK_MODEL_ATAC_CLUSTER_H__

#include <string>
using namespace std;

#include "queue_model.h"
#include "network.h"
#include "lock.h"

// Single Sender Multiple Receivers Model
// 1 sender, N receivers (1 to N)
class NetworkModelAtacCluster : public NetworkModel
{
   private:
      enum HubType
      {
         SENDER_HUB = 0,
         RECEIVER_HUB,
         NUM_HUB_TYPES
      };

      // Latency Parameters
      UInt64 m_optical_hop_latency;
      UInt64 m_electrical_mesh_hop_latency;
      UInt64 m_sender_cluster_electrical_network_delay;
      UInt64 m_receiver_cluster_electrical_network_delay;

      // Bandwidth Parameters
      UInt32 m_optical_bus_bandwidth;
      UInt32 m_num_electrical_broadcast_networks_per_cluster;

      core_id_t m_core_id;
      UInt32 m_num_application_cores;
      UInt32 m_num_clusters;
      UInt32 m_total_cores;

      // Optical Hub models
      QueueModel* m_sender_hub_queue_model;
      QueueModel** m_receiver_hub_queue_models;
      // Locks for the Queue Models
      Lock m_sender_hub_lock;
      Lock m_receiver_hub_lock;

      // Topology
      static SInt32 m_sqrt_cluster_size;
      static SInt32 m_mesh_width;
      static SInt32 m_mesh_height;

      bool m_queue_model_enabled;
      string m_queue_model_type;
      bool m_enabled;

      // Lock
      Lock m_lock;

      // Performance Counters
      UInt64 m_total_bytes_received;
      UInt64 m_total_packets_received;
      UInt64 m_total_packet_latency;

      UInt64 m_total_sender_hub_contention_delay;
      UInt64 m_total_sender_hub_packets;

      UInt64* m_total_receiver_hub_contention_delay;
      UInt64* m_total_receiver_hub_packets;

      static SInt32 getClusterID(core_id_t core_id);
      static core_id_t getCoreIDWithOpticalHub(SInt32 cluster_id);

      void createOpticalHub();
      void destroyOpticalHub();
      UInt64 getHubQueueDelay(HubType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id,
            UInt64 pkt_time, UInt32 pkt_length, PacketType pkt_type, core_id_t requester);
      void outputHubSummary(ostream& out);

      UInt64 computeProcessingTime(UInt32 pkt_length, UInt32 bandwidth);
      core_id_t getRequester(const NetPacket& pkt);

      void initializePerformanceCounters();

   public:
      NetworkModelAtacCluster(Network *net);
      ~NetworkModelAtacCluster();

      void routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops);
      void processReceivedPacket(NetPacket& pkt);
      static pair<bool, vector<core_id_t> > computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count);
      
      void enable();
      void disable();
      void outputSummary(std::ostream &out);
      
      // Only for NetworkModelAtacCluster
      UInt64 computeHubQueueDelay(HubType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, UInt32 pkt_length);
};

#endif /* __NETWORK_MODEL_ATAC_CLUSTER_H__ */
