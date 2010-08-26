#include <math.h>
#include <vector>
using namespace std;

#include "network_model_atac_cluster.h"
#include "simulator.h"
#include "core_manager.h"
#include "core.h"
#include "config.h"
#include "log.h"
#include "memory_manager_base.h"
#include "packet_type.h"
#include "clock_converter.h"
#include "utils.h"

SInt32 NetworkModelAtacCluster::m_cluster_size = 0;
SInt32 NetworkModelAtacCluster::m_sqrt_cluster_size = 0;
SInt32 NetworkModelAtacCluster::m_mesh_width = 0;
SInt32 NetworkModelAtacCluster::m_mesh_height = 0;

NetworkModelAtacCluster::NetworkModelAtacCluster(Network *net, SInt32 network_id):
   NetworkModel(net, network_id),
   m_enabled(false)
{
   m_core_id = getNetwork()->getCore()->getId();
  
   // Initialize Cluster size, num clusters, etc
   initializeANetTopologyParams();

   // Initialize ENet, ONet and BNet parameters
   createANetLinkModels();

   // Optical Hub created only on one of the cores in the cluster
   createOpticalHub();

   // Performance Counters
   initializePerformanceCounters();
}

NetworkModelAtacCluster::~NetworkModelAtacCluster()
{
   // Destroy the Optical hub created on one of the cores in the cluster
   destroyOpticalHub();

   // Destroy the Link Models
   destroyANetLinkModels();
}

void
NetworkModelAtacCluster::initializeANetTopologyParams()
{
   m_total_cores = Config::getSingleton()->getTotalCores();

   try
   {
      m_cluster_size = Sim()->getCfg()->getInt("network/atac_cluster/cluster_size");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Error reading atac cluster size");
   }

   // Cluster Size
   m_sqrt_cluster_size = (SInt32) floor(sqrt(m_cluster_size));
   LOG_ASSERT_ERROR(m_cluster_size == (m_sqrt_cluster_size * m_sqrt_cluster_size),
         "Cluster Size(%i) must be a perfect square", m_cluster_size);

   // Calculations with an electrical mesh
   m_mesh_width = (SInt32) floor(sqrt(m_total_cores));
   m_mesh_height = (SInt32) ceil(1.0 * m_total_cores / m_mesh_width);
   LOG_ASSERT_ERROR(m_mesh_width % m_sqrt_cluster_size == 0, \
         "Mesh Width(%i) must be a multiple of sqrt_cluster_size(%i)", \
         m_mesh_width, m_sqrt_cluster_size);
   LOG_ASSERT_ERROR(m_mesh_height == (m_mesh_width + 1),
         "Mesh Width(%i), Mesh Height(%i)", m_mesh_width, m_mesh_height);
   LOG_ASSERT_ERROR((m_mesh_width * m_mesh_height) == (SInt32) m_total_cores,
         "Mesh Width(%i), Mesh Height(%i), Core Count(%i)", m_mesh_width, m_mesh_height, m_total_cores);

   // Number of Clusters
   m_num_clusters = (m_mesh_width / m_sqrt_cluster_size) * ceil(1.0 * m_mesh_height / m_sqrt_cluster_size);
}

void
NetworkModelAtacCluster::createANetLinkModels()
{
   // 1) Initialize Frequency Parameters
   // Everything is normalized to the frequency of the ENet - the frequency of the network
   // is taken as the frequency of the ENet
   try
   {
      m_gather_network_frequency = Sim()->getCfg()->getFloat("network/atac_cluster/enet/frequency");
      m_optical_network_frequency = Sim()->getCfg()->getFloat("network/atac_cluster/onet/frequency");
      m_scatter_network_frequency = Sim()->getCfg()->getFloat("network/atac_cluster/bnet/frequency");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet frequency parameters from the cfg file");
   }
   
   // Currently assume gather_network and scatter_network have the same frequency
   LOG_ASSERT_ERROR(m_gather_network_frequency == m_scatter_network_frequency, \
         "Currently, ENet and BNet should have the same frequency, \
         specified frequencies: ENet(%f), BNet(%f)", \
         m_gather_network_frequency, m_scatter_network_frequency);

   // 2) Initialize Bandwidth Parameters
   try
   {
      m_gather_network_link_width = Sim()->getCfg()->getInt("network/atac_cluster/enet/link/width");
      m_optical_network_link_width = Sim()->getCfg()->getInt("network/atac_cluster/onet/link/width");
      m_scatter_network_link_width = Sim()->getCfg()->getInt("network/atac_cluster/bnet/link/width");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet link width parameters from the cfg file");
   }
   
   m_gather_network_bandwidth = static_cast<double>(m_gather_network_link_width);
   m_optical_network_bandwidth = static_cast<double>(m_optical_network_link_width) * (m_optical_network_frequency / m_gather_network_frequency);
   m_scatter_network_bandwidth = static_cast<double>(m_scatter_network_link_width) * (m_scatter_network_frequency / m_gather_network_frequency);
   m_effective_anet_bandwidth = getMin<double>(m_gather_network_bandwidth, m_optical_network_bandwidth, m_scatter_network_bandwidth);

   // 3) Initialize Latency Parameters
   volatile double gather_network_link_length;
   UInt32 gather_network_router_delay = 0;
   volatile double waveguide_length;
   volatile double scatter_network_link_length;
   try
   {
      // Number of Hops from the sender core to sender hub
      m_num_hops_sender_core_to_sender_hub = Sim()->getCfg()->getInt("network/atac_cluster/enet/num_hops_to_hub");
      // Router Delay of the electrical mesh network - Specified in clock cycles
      gather_network_router_delay = Sim()->getCfg()->getInt("network/atac_cluster/enet/router/delay");
      // Length of a gather delay link
      gather_network_link_length = Sim()->getCfg()->getFloat("network/atac_cluster/enet/link/length");

      // Waveguide length of the optical network
      waveguide_length = Sim()->getCfg()->getFloat("network/atac_cluster/onet/link/length");
     
      // Length of a scatter network link (for the purpose of modeling power) 
      scatter_network_link_length = Sim()->getCfg()->getFloat("network/atac_cluster/bnet/link/length");
      // Specified in terms of clock cycles where the clock frequency = scatter_network_frequency
      // Delay is the delay of one scatter_network network. So, you need to pay just this delay when
      // going from the receiver hub to the receiver core
      m_scatter_network_delay = Sim()->getCfg()->getInt("network/atac_cluster/bnet/network_delay");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet link delay and length parameters from the cfg file");
   }

   // 4) Link type parameters
   try
   {
      m_gather_network_link_type = Sim()->getCfg()->getString("network/atac_cluster/enet/link/type");
      m_scatter_network_link_type = Sim()->getCfg()->getString("network/atac_cluster/bnet/link/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet link type parameters from the cfg file");
   }

   m_optical_network_link_model = new OpticalNetworkLinkModel(m_optical_network_frequency, \
         waveguide_length, \
         m_optical_network_link_width);
   m_optical_network_link_delay = (UInt64) (ceil( ((double) m_optical_network_link_model->getDelay()) / \
                      (m_optical_network_frequency / m_gather_network_frequency) ) );

   m_gather_network_link_model = ElectricalNetworkLinkModel::create(m_gather_network_link_type, \
         m_gather_network_frequency, \
         gather_network_link_length, \
         m_gather_network_link_width);
   // Specified in terms of clock cycles where the clock frequency = gather_network frequency
   // Delay is the delay of one gather_network link
   // There may be multiple gather_network links from the core to the sending hub
   m_gather_network_link_delay = m_gather_network_link_model->getDelay();
   // m_gather_network_link_delay is already initialized
   m_gather_network_delay = (m_gather_network_link_delay + gather_network_router_delay) * m_num_hops_sender_core_to_sender_hub;

   // FIXME: Currently, the BNet network power is modeled using multiple scatter_network links.
   // This is not correct. It has to be changed
   m_scatter_network_link_model = ElectricalNetworkLinkModel::create(m_scatter_network_link_type, \
         m_scatter_network_frequency, \
         scatter_network_link_length, \
         m_scatter_network_link_width);
   // m_scatter_network_delay is already initialized. Conversion needed here
   m_scatter_network_delay = (UInt64) (ceil( ((double) m_scatter_network_delay) / \
                      (m_scatter_network_frequency / m_gather_network_frequency) ) );
}

void
NetworkModelAtacCluster::destroyANetLinkModels()
{
   delete m_gather_network_link_model;
   delete m_optical_network_link_model;
   delete m_scatter_network_link_model;
}

void
NetworkModelAtacCluster::createOpticalHub()
{
   try
   {
      m_num_scatter_networks_per_cluster = Sim()->getCfg()->getInt("network/atac_cluster/num_bnets_per_cluster");
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/atac_cluster/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/atac_cluster/queue_model/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read cluster and queue model parameters from the cfg file");
   }

   if (m_queue_model_enabled && (m_core_id == getCoreIDWithOpticalHub(getClusterID(m_core_id))))
   {
      // I am one of the cores with an optical hub

      UInt64 min_processing_time = 1;
     
      // Create Sending Hub 
      m_sender_hub_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
      m_total_sender_hub_contention_delay = 0;
      m_total_sender_hub_packets = 0; 

      // Create Receiving Hub 
      m_receiver_hub_queue_models = new QueueModel*[m_num_scatter_networks_per_cluster];
      m_total_receiver_hub_contention_delay = new UInt64[m_num_scatter_networks_per_cluster];
      m_total_receiver_hub_packets = new UInt64[m_num_scatter_networks_per_cluster];
      
      for (SInt32 i = 0; i < (SInt32) m_num_scatter_networks_per_cluster; i++)
      {
         m_receiver_hub_queue_models[i] = QueueModel::create(m_queue_model_type, min_processing_time);
         m_total_receiver_hub_contention_delay[i] = 0;
         m_total_receiver_hub_packets[i] = 0;
      }
   }
   else
   {
      m_sender_hub_queue_model = (QueueModel*) NULL;
      m_receiver_hub_queue_models = (QueueModel**) NULL;
   }
}

void
NetworkModelAtacCluster::destroyOpticalHub()
{
   if ((m_queue_model_enabled) && (m_core_id == getCoreIDWithOpticalHub(getClusterID(m_core_id))))
   {
      delete m_sender_hub_queue_model;
      
      for (SInt32 i = 0; i < (SInt32) m_num_scatter_networks_per_cluster; i++)
         delete m_receiver_hub_queue_models[i];
      delete m_receiver_hub_queue_models;

      delete m_total_receiver_hub_contention_delay;
      delete m_total_receiver_hub_packets;
   }
}

void
NetworkModelAtacCluster::initializePerformanceCounters()
{
   m_total_bytes_received = 0;
   m_total_packets_received = 0;
   m_total_packet_latency = 0;
}

UInt64 
NetworkModelAtacCluster::computeProcessingTime(UInt32 pkt_length, volatile double bandwidth)
{
   return static_cast<UInt64>(ceil(static_cast<double>(pkt_length * 8) / bandwidth));
}

core_id_t
NetworkModelAtacCluster::getRequester(const NetPacket& pkt)
{
   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;

   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);
   
   return requester;
}

UInt64
NetworkModelAtacCluster::getHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id, UInt64 pkt_time, UInt32 pkt_length, PacketType pkt_type, core_id_t requester)
{
   core_id_t core_id_with_optical_hub = getCoreIDWithOpticalHub(cluster_id);
   Core* core = Sim()->getCoreManager()->getCoreFromID(core_id_with_optical_hub);
   NetworkModelAtacCluster* network_model = (NetworkModelAtacCluster*) core->getNetwork()->getNetworkModelFromPacketType(pkt_type);

   return network_model->computeHubQueueDelay(hub_type, sender_cluster_id, pkt_time, pkt_length, requester);
}

UInt64
NetworkModelAtacCluster::computeHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, UInt32 pkt_length, core_id_t requester)
{
   if ((!m_enabled) || (!m_queue_model_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
      return 0;

   assert(m_core_id == getCoreIDWithOpticalHub(getClusterID(m_core_id)));
   assert(m_sender_hub_queue_model);
   assert(m_receiver_hub_queue_models);

   switch (hub_type)
   {
      case SENDER_HUB:
         {
            ScopedLock sl(m_sender_hub_lock);

            // Convert from gather network clock to optical network clock
            UInt64 optical_network_pkt_time = convertCycleCount(pkt_time, m_gather_network_frequency, m_optical_network_frequency);
            
            UInt64 processing_time = computeProcessingTime(pkt_length, (volatile double) m_optical_network_link_width);

            LOG_ASSERT_ERROR(sender_cluster_id == getClusterID(m_core_id), \
                  "sender_cluster_id(%i), curr_cluster_id(%i)", \
                  sender_cluster_id, getClusterID(m_core_id));
            UInt64 sender_hub_queue_delay = m_sender_hub_queue_model->computeQueueDelay(optical_network_pkt_time, processing_time);

            // Performance Counters
            m_total_sender_hub_contention_delay += sender_hub_queue_delay;
            m_total_sender_hub_packets ++;

            // Also convert from optical network clock to gather network clock
            return convertCycleCount(sender_hub_queue_delay, m_optical_network_frequency, m_gather_network_frequency);
         }

      case RECEIVER_HUB:
         {
            ScopedLock sl(m_receiver_hub_lock);
           
            // Convert from gather network clock to scatter network clock
            UInt64 scatter_network_pkt_time = convertCycleCount(pkt_time, m_gather_network_frequency, m_scatter_network_frequency);

            UInt64 processing_time = computeProcessingTime(pkt_length, (volatile double) m_scatter_network_link_width);

            // Assume the broadcast networks are statically divided up among the senders
            SInt32 scatter_network_num = sender_cluster_id % m_num_scatter_networks_per_cluster;
            UInt64 receiver_hub_queue_delay = m_receiver_hub_queue_models[scatter_network_num]->computeQueueDelay(scatter_network_pkt_time, processing_time);

            // Performance Counters
            m_total_receiver_hub_contention_delay[scatter_network_num] += receiver_hub_queue_delay;
            m_total_receiver_hub_packets[scatter_network_num] ++;
            
            // Also convert from scatter network clock to gather network clock
            return convertCycleCount(receiver_hub_queue_delay, m_scatter_network_frequency, m_gather_network_frequency);
         }

      default:
         LOG_PRINT_ERROR("Network Component(%u) not a Hub", hub_type);
         return 0;
   }
}

UInt32
NetworkModelAtacCluster::computeAction(const NetPacket& pkt)
{
   // If this endpoint is a hub, FORWARD it, else RECEIVE it
   // FORWARD may mean forward it to mutiple recipients too depending
   // on the routing function
   if ((pkt.specific == SENDER_HUB) || (pkt.specific == RECEIVER_HUB))
   {
      return RoutingAction::FORWARD;
   }
   else if (pkt.specific == RECEIVER_CORE)
   {
      LOG_ASSERT_ERROR((pkt.receiver == NetPacket::BROADCAST) || (pkt.receiver == m_core_id),
            "pkt.receiver(%i)", pkt.receiver);
      return RoutingAction::RECEIVE;
   }
   else
   {
      LOG_PRINT_ERROR("Unhandled sub network type(%u)", pkt.specific);
      return RoutingAction::DROP;
   }
}

void 
NetworkModelAtacCluster::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

   core_id_t requester = getRequester(pkt);
      
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   UInt64 serialization_delay = computeProcessingTime(pkt_length, m_effective_anet_bandwidth);

   if (Config::getSingleton()->getProcessCount() == 1)
   {
      // All the energies and delays are computed at source
      if (pkt.receiver == NetPacket::BROADCAST)
      {
         updateDynamicEnergy(GATHER_NETWORK, pkt);
         updateDynamicEnergy(OPTICAL_NETWORK, pkt);
         
         UInt64 sender_hub_queue_delay = getHubQueueDelay(SENDER_HUB, \
               getClusterID(pkt.sender), \
               getClusterID(pkt.sender), \
               pkt.time + m_gather_network_delay, \
               pkt_length, \
               pkt.type, requester);
         UInt64 latency_sender_core_to_receiver_hub = m_gather_network_delay + \
                                                      sender_hub_queue_delay + \
                                                      m_optical_network_link_delay;

         UInt64 receiver_hub_queue_delay[m_num_clusters];
         for (SInt32 i = 0; i < (SInt32) m_num_clusters; i++)
         {
            updateDynamicEnergy(SCATTER_NETWORK, pkt);
            
            receiver_hub_queue_delay[i] = getHubQueueDelay(RECEIVER_HUB, \
                  getClusterID(pkt.sender), \
                  i, \
                  pkt.time + latency_sender_core_to_receiver_hub, \
                  pkt_length, \
                  pkt.type, requester);
         }

         for (core_id_t i = 0; i < (core_id_t) m_total_cores; i++)
         {
            UInt64 latency_receiver_hub_to_receiver_core = receiver_hub_queue_delay[getClusterID(i)] + \
                                                           m_scatter_network_delay;

            UInt64 total_latency = latency_sender_core_to_receiver_hub + \
                                   latency_receiver_hub_to_receiver_core + \
                                   serialization_delay;

            Hop h;
            h.next_dest = i;
            h.final_dest = NetPacket::BROADCAST;
            h.specific = RECEIVER_CORE;
            h.time = pkt.time + total_latency;
            
            nextHops.push_back(h);
         }
      }
      else // if (pkt.receiver != NetPacket::BROADCAST)
      {
         // Right now, it is a unicast/multicast
         LOG_ASSERT_ERROR(pkt.receiver < (core_id_t) m_total_cores, \
               "Got invalid receiver ID = %i", pkt.receiver);

         UInt64 total_latency;
         if (pkt.sender == pkt.receiver)
         {
            total_latency = 0;
         }
         else
         {
            updateDynamicEnergy(GATHER_NETWORK, pkt);

            UInt64 latency_sender_core_to_receiver_hub;
            if (getClusterID(pkt.sender) == getClusterID(pkt.receiver))
            {
               latency_sender_core_to_receiver_hub = m_gather_network_delay;
            }
            else
            {
               updateDynamicEnergy(OPTICAL_NETWORK, pkt);
               
               UInt64 sender_hub_queue_delay = getHubQueueDelay(SENDER_HUB, \
                     getClusterID(pkt.sender), \
                     getClusterID(pkt.sender), \
                     pkt.time + m_gather_network_delay, \
                     pkt_length, \
                     pkt.type, requester);
               latency_sender_core_to_receiver_hub = m_gather_network_delay + \
                                                     sender_hub_queue_delay + \
                                                     m_optical_network_link_delay;
            }

            updateDynamicEnergy(SCATTER_NETWORK, pkt);

            UInt64 receiver_hub_queue_delay = getHubQueueDelay(RECEIVER_HUB, \
                  getClusterID(pkt.sender), \
                  getClusterID(pkt.receiver), \
                  pkt.time + latency_sender_core_to_receiver_hub, \
                  pkt_length, \
                  pkt.type, requester);
            UInt64 latency_receiver_hub_to_receiver_core = receiver_hub_queue_delay + \
                                                           m_scatter_network_delay;

            total_latency = latency_sender_core_to_receiver_hub + \
                            latency_receiver_hub_to_receiver_core + \
                            serialization_delay;
         }

         Hop h;
         h.next_dest = pkt.receiver;
         h.final_dest = pkt.receiver;
         h.specific = RECEIVER_CORE;
         h.time = pkt.time + total_latency;
         
         nextHops.push_back(h);
      }
   }
   else // Config::getSingleton()->getProcessCount() > 1
   {
      // Process Count > 1
      // Let me have at least 2 hops within the same process
      // and 1 hop between processes
      if (pkt.receiver == NetPacket::BROADCAST)
      {
         if (pkt.specific == SENDER_HUB)
         {
            // Sender Hub
            // 1) Send it to all the other Hubs on the chip
            // 2) Broadcast it to all the cores within the same cluster
            // Compute:
            // 1) Sender Hub queue delay
            
            // Optical Link Traversal
            updateDynamicEnergy(OPTICAL_NETWORK, pkt);

            UInt64 sender_hub_queue_delay = computeHubQueueDelay(SENDER_HUB, \
                  getClusterID(pkt.sender), \
                  pkt.time, \
                  pkt_length, \
                  requester);
            
            // First, send it to all the other hubs
            for (SInt32 i = 0; i < (SInt32) m_num_clusters; i++)
            {
               if (i != getClusterID(m_core_id))
               {
                  Hop h;
                  h.next_dest = getCoreIDWithOpticalHub(i);
                  h.final_dest = NetPacket::BROADCAST;
                  h.specific = RECEIVER_HUB;
                  h.time = pkt.time + sender_hub_queue_delay + m_optical_network_link_delay;

                  nextHops.push_back(h);
               }
            }

            // Broadcast Network Traversal
            updateDynamicEnergy(SCATTER_NETWORK, pkt);

            // Second, send it to all the cores within the same cluster
            // We need to compute receiver hub queue delay here
            UInt64 receiver_hub_queue_delay = computeHubQueueDelay(RECEIVER_HUB, \
                  getClusterID(pkt.sender), \
                  pkt.time, \
                  pkt_length, \
                  requester);

            vector<core_id_t> core_id_list_in_cluster;
            getCoreIDListInCluster(getClusterID(m_core_id), core_id_list_in_cluster);
            for (vector<core_id_t>::iterator core_it = core_id_list_in_cluster.begin(); \
                  core_it != core_id_list_in_cluster.end(); core_it++)
            {
               Hop h;
               h.next_dest = *core_it;
               h.final_dest = *core_it;
               h.specific = RECEIVER_CORE;
               h.time = pkt.time + receiver_hub_queue_delay + m_scatter_network_delay;

               nextHops.push_back(h);
            }
         }
         
         else if (pkt.specific == RECEIVER_HUB)
         {
            // Receiver Hub
            
            // Broadcast Network Traversal
            updateDynamicEnergy(SCATTER_NETWORK, pkt);

            // Send it to all the cores within the same cluster
            // We need to compute receiver hub queue delay here
            UInt64 receiver_hub_queue_delay = computeHubQueueDelay(RECEIVER_HUB, \
                  getClusterID(pkt.sender), \
                  pkt.time, \
                  pkt_length, \
                  requester);

            vector<core_id_t> core_id_list_in_cluster;
            getCoreIDListInCluster(getClusterID(m_core_id), core_id_list_in_cluster);
            for (vector<core_id_t>::iterator core_it = core_id_list_in_cluster.begin(); \
                  core_it != core_id_list_in_cluster.end(); core_it++)
            {
               Hop h;
               h.next_dest = *core_it;
               h.final_dest = *core_it;
               h.specific = RECEIVER_CORE;
               h.time = pkt.time + receiver_hub_queue_delay + m_scatter_network_delay;

               nextHops.push_back(h);
            }
         }

         else if (pkt.specific == INVALID_COMPONENT)
         {
            // Must be the sender. pkt.specific is not set
            LOG_ASSERT_ERROR(pkt.sender == m_core_id, "pkt.sender(%i), m_core_id(%i)",
                  pkt.sender, m_core_id);
               
            // Sender Cluster Electrical Network(ENet) traversal
            updateDynamicEnergy(GATHER_NETWORK, pkt);

            // I am the sender of the broadcast packet
            // I dont have an associated optical hub
            // Send it to the core having the hub associated with the current cluster
            Hop h;
            h.next_dest = getCoreIDWithOpticalHub(getClusterID(m_core_id));
            h.final_dest = NetPacket::BROADCAST;
            h.specific = SENDER_HUB;
            h.time = pkt.time + m_gather_network_delay;

            nextHops.push_back(h);
         }

         else // pkt.specific is not one of the known values
         {
            LOG_PRINT_ERROR("Routing Function not defined: pkt.sender(%i), \
                  pkt.receiver(%i), pkt.specific(%u), pkt.time(%llu)",
                  pkt.sender, pkt.receiver, pkt.specific, pkt.time);
         }
      }

      else // pkt.receiver != NetPacket::BROADCAST
      {
         // Unicasted/Multicasted packet
         LOG_ASSERT_ERROR(pkt.receiver < (core_id_t) m_total_cores, \
               "Got invalid receiver ID = %i", pkt.receiver);

         UInt64 hop_latency = 0;
         core_id_t next_dest = INVALID_CORE_ID;
         UInt32 next_component = 0;

         if (pkt.specific == SENDER_HUB)
         {
            // Is receiver in the same cluster
            if (getClusterID(pkt.sender) == getClusterID(pkt.receiver))
            {
               // BNet traversal
               updateDynamicEnergy(SCATTER_NETWORK, pkt);

               // Yes, calculate the receiver hub queue delay
               UInt64 receiver_hub_queue_delay = computeHubQueueDelay(RECEIVER_HUB, \
                   getClusterID(pkt.sender), \
                   pkt.time, \
                   pkt_length, \
                   requester);

               hop_latency = (receiver_hub_queue_delay + m_scatter_network_delay);
               next_dest = pkt.receiver;
               next_component = RECEIVER_CORE;
            }
            else // getClusterID(pkt.sender) != getClusterID(pkt.receiver)
            {
               // ONet traversal
               updateDynamicEnergy(OPTICAL_NETWORK, pkt);

               // Receiving cluster different from sending cluster
               UInt64 sender_hub_queue_delay = computeHubQueueDelay(SENDER_HUB, \
                    getClusterID(pkt.sender), \
                    pkt.time, \
                    pkt_length, \
                    requester);

               hop_latency = (sender_hub_queue_delay + m_optical_network_link_delay);
               next_dest = getCoreIDWithOpticalHub(getClusterID(pkt.receiver));
               next_component = RECEIVER_HUB;
            }
         }

         else if (pkt.specific == RECEIVER_HUB)
         {
            // BNet traversal
            updateDynamicEnergy(SCATTER_NETWORK, pkt);

            // Receiver Hub
            // Forward the packet to the actual receiver
            UInt64 receiver_hub_queue_delay = computeHubQueueDelay(RECEIVER_HUB, \
                  getClusterID(pkt.sender), \
                  pkt.time, \
                  pkt_length, \
                  requester);

            LOG_ASSERT_ERROR(getClusterID(m_core_id) == getClusterID(pkt.receiver), \
                  "m_core_id(%i), cluster(%u), pkt.receiver(%i), cluster(%u)", \
                  m_core_id, getClusterID(m_core_id), pkt.receiver, getClusterID(pkt.receiver));

            hop_latency = (receiver_hub_queue_delay + m_scatter_network_delay);
            next_dest = pkt.receiver;
            next_component = RECEIVER_CORE;
         }

         else if (pkt.specific == INVALID_COMPONENT)
         {
            // pkt.specific is not set. Must be the sender
            LOG_ASSERT_ERROR(pkt.sender == m_core_id, "pkt.sender(%i), m_core_id(%i)",
                  pkt.sender, m_core_id);
            
            if (pkt.sender == pkt.receiver)
            {
               hop_latency = 0;
               next_dest = pkt.receiver;
               next_component = RECEIVER_CORE;
            }
            else
            {
               // ENet traversal
               updateDynamicEnergy(GATHER_NETWORK, pkt);

               // Sending node - I am not optical hub
               hop_latency = m_gather_network_delay;
               next_dest = getCoreIDWithOpticalHub(getClusterID(pkt.sender));
               next_component = SENDER_HUB;
            }
         }
         
         else
         {
            LOG_PRINT_ERROR("Unrecognized Routing Function: pkt.sender(%i), \
                  pkt.receiver(%i), pkt.specific(%u), m_core_id(%i)",
                  pkt.sender, pkt.receiver, pkt.specific, m_core_id);
         }

         if (next_dest != INVALID_CORE_ID)
         {
            Hop h;
            h.next_dest = next_dest;
            h.final_dest = pkt.receiver;
            h.specific = next_component;
            h.time = pkt.time + hop_latency;

            nextHops.push_back(h);
         }

      } // if (pkt.receiver == NetPacket::BROADCAST)

   } // if (Config::getSingleton()->getProcessCount() == 1)
}

void
NetworkModelAtacCluster::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);
 
   core_id_t requester = getRequester(pkt);

   if ((!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()))
      return;

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   
   if (Config::getSingleton()->getProcessCount() > 1)
   {
      // Serialization Delay
      pkt.time += computeProcessingTime(pkt_length, m_effective_anet_bandwidth); 
   }
   
   // Performance Counters
   m_total_packets_received ++;
   
   m_total_bytes_received += (UInt64) pkt_length;
   
   UInt64 packet_latency = pkt.time - pkt.start_time;
   m_total_packet_latency += packet_latency;
}

void
NetworkModelAtacCluster::outputHubSummary(ostream& out)
{
   out << " ATAC Cluster: " << endl;
   if ((m_queue_model_enabled) && (m_core_id == getCoreIDWithOpticalHub(getClusterID(m_core_id))))
   {
      if (m_total_sender_hub_packets > 0)
      {
         // Convert from optical network clock to global clock
         UInt64 total_sender_hub_contention_delay_in_ns = convertCycleCount( \
               m_total_sender_hub_contention_delay, \
               m_optical_network_frequency, 1.0);
         out << "    Sender Hub Contention Delay (in ns): " << \
            ((float) total_sender_hub_contention_delay_in_ns) / m_total_sender_hub_packets << endl;
      }
      else
      {
         out << "    Sender Hub Contention Delay (in ns): 0" << endl;
      }
    
      for (UInt32 i = 0; i < m_num_scatter_networks_per_cluster; i++)
      {
         if (m_total_receiver_hub_packets[i] > 0)
         {
            // Convert from scatter network clock to global clock
            UInt64 total_receiver_hub_contention_delay_in_ns = convertCycleCount( \
                  m_total_receiver_hub_contention_delay[i], \
                  m_scatter_network_frequency, 1.0);
            out << "    Receiver Hub (" << i << ") Contention Delay (in ns): " \
               << ((float) total_receiver_hub_contention_delay_in_ns) / m_total_receiver_hub_packets[i] << endl;
         }
         else
         {
            out << "    Receiver Hub (" << i << ") Contention Delay (in ns): 0" << endl;
         }
      }
   }
   else
   {
      out << "    Sender Hub Contention Delay (in ns): NA" << endl;
      for (UInt32 i = 0; i < m_num_scatter_networks_per_cluster; i++)
      {
         out << "    Receiver Hub (" << i << ") Contention Delay (in ns): NA" << endl;
      }
   }
}

void
NetworkModelAtacCluster::outputSummary(ostream &out)
{
   out << "    bytes received: " << m_total_bytes_received << endl;
   out << "    packets received: " << m_total_packets_received << endl;
   if (m_total_packets_received > 0)
   {
      // Convert from gather network clock to global clock
      UInt64 total_packet_latency_in_ns = convertCycleCount(m_total_packet_latency, \
            m_gather_network_frequency, 1.0);
      out << "    average packet latency (in ns): " << \
         ((float) total_packet_latency_in_ns) / m_total_packets_received << endl;
   }
   else
   {
      out << "    average packet latency (in ns): 0" << endl;
   }

   outputHubSummary(out);
   outputPowerSummary(out);
}

void
NetworkModelAtacCluster::enable()
{
   m_enabled = true;
}

void
NetworkModelAtacCluster::disable()
{
   m_enabled = false;
}

pair<bool, vector<core_id_t> >
NetworkModelAtacCluster::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count)
{
   // Just a check to see if it is initialized
   assert(m_sqrt_cluster_size > 0);
   assert(m_mesh_width > 0);

   // Get the cluster size, Get the number of clusters
   // Here we only include complete clusters, we dont include the incomplete clusters
   
   SInt32 num_clusters = (m_mesh_width / m_sqrt_cluster_size) * (m_mesh_width / m_sqrt_cluster_size);
   LOG_ASSERT_ERROR(num_memory_controllers <= num_clusters,
         "num_memory_controllers(%i), num_clusters(%i)", num_memory_controllers, num_clusters);

   vector<core_id_t> core_id_list_with_memory_controllers;
   for (SInt32 i = 0; i < num_memory_controllers; i++)
   {
      core_id_list_with_memory_controllers.push_back(getCoreIDWithOpticalHub(i));
   }

   return (make_pair(true, core_id_list_with_memory_controllers));
}

SInt32
NetworkModelAtacCluster::getClusterID(core_id_t core_id)
{
   // Consider a mesh formed by the clusters
   SInt32 cluster_mesh_width;
   cluster_mesh_width = m_mesh_width / m_sqrt_cluster_size;

   SInt32 core_x, core_y;
   core_x = core_id % m_mesh_width;
   core_y = core_id / m_mesh_width;

   SInt32 cluster_pos_x, cluster_pos_y;
   cluster_pos_x = core_x / m_sqrt_cluster_size;
   cluster_pos_y = core_y / m_sqrt_cluster_size;

   return (cluster_pos_y * cluster_mesh_width + cluster_pos_x);
}

core_id_t
NetworkModelAtacCluster::getCoreIDWithOpticalHub(SInt32 cluster_id)
{
   // Consider a mesh formed by the clusters
   SInt32 cluster_mesh_width;
   cluster_mesh_width = m_mesh_width / m_sqrt_cluster_size;

   SInt32 cluster_pos_x, cluster_pos_y;
   cluster_pos_x = cluster_id % cluster_mesh_width;
   cluster_pos_y = cluster_id / cluster_mesh_width;

   SInt32 optical_hub_x, optical_hub_y; 
   optical_hub_x = cluster_pos_x * m_sqrt_cluster_size;
   optical_hub_y = cluster_pos_y * m_sqrt_cluster_size;

   return (optical_hub_y * m_mesh_width + optical_hub_x);
}

void
NetworkModelAtacCluster::getCoreIDListInCluster(SInt32 cluster_id, vector<core_id_t>& core_id_list)
{
   SInt32 cluster_mesh_width;
   cluster_mesh_width = m_mesh_width / m_sqrt_cluster_size;

   SInt32 cluster_pos_x, cluster_pos_y;
   cluster_pos_x = cluster_id % cluster_mesh_width;
   cluster_pos_y = cluster_id / cluster_mesh_width;

   SInt32 optical_hub_x, optical_hub_y; 
   optical_hub_x = cluster_pos_x * m_sqrt_cluster_size;
   optical_hub_y = cluster_pos_y * m_sqrt_cluster_size;

   for (SInt32 i = optical_hub_x; i < optical_hub_x + m_sqrt_cluster_size; i++)
   {
      for (SInt32 j = optical_hub_y; j < optical_hub_y + m_sqrt_cluster_size; j++)
      {
         SInt32 core_id = j * m_mesh_width + i;
         if (core_id < (SInt32) Config::getSingleton()->getTotalCores())
            core_id_list.push_back(core_id);
      }
   }
}

void
NetworkModelAtacCluster::updateDynamicEnergy(SubNetworkType sub_net_type, const NetPacket& pkt)
{
   if (!Config::getSingleton()->getEnablePowerModeling())
      return;

   // TODO: Make these models more detailed later - Compute the exact number of bit flips
   // FIXME: m_cluster_size is the number of links currently in the scatter network and
   // the gather network
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   switch(sub_net_type)
   {
      case GATHER_NETWORK:
         // TODO: Check if the number of gather network links traversed is correct
         // We basically look at a mesh and compute the average number of links traversed
         m_gather_network_link_model->updateDynamicEnergy(m_gather_network_link_width / 2, \
               computeProcessingTime(pkt_length, m_gather_network_link_width) * (m_sqrt_cluster_size/2 + 1));
         break;

      case OPTICAL_NETWORK:
         m_optical_network_link_model->updateDynamicEnergy(m_optical_network_link_width / 2, \
               computeProcessingTime(pkt_length, m_optical_network_link_width));
         break;

      case SCATTER_NETWORK:
         m_scatter_network_link_model->updateDynamicEnergy(m_scatter_network_link_width / 2, \
               computeProcessingTime(pkt_length, m_scatter_network_link_width) * m_cluster_size);
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized Sub Network Type(%u)", sub_net_type);
         break;
   }
}

void
NetworkModelAtacCluster::outputPowerSummary(ostream& out)
{
   if (!Config::getSingleton()->getEnablePowerModeling())
      return;

   volatile double total_static_power = 0.0;
   if (m_core_id == getCoreIDWithOpticalHub(getClusterID(m_core_id)))
   {
      // I have an optical Hub
      // m_cluster_size is the number of cores in the cluster + 1 Hub
      // So, a total of (m_cluster_size + 1) nodes
      // We need m_cluster_size edges to connect them to form a tree

      // Get the static power for the entire network.
      // This function will only be called only on Core0

      // FIXME: Correct the number of gather network links
      // Both the gather and scatter networks are modeled as H-trees now
      UInt32 total_gather_network_links = m_cluster_size;
      volatile double static_power_gather_network = m_gather_network_link_model->getStaticPower() * total_gather_network_links;

      volatile double static_power_optical_network = m_optical_network_link_model->getStaticPower();

      UInt32 total_scatter_network_links = m_cluster_size;
      volatile double static_power_scatter_network = m_scatter_network_link_model->getStaticPower() * total_scatter_network_links;

      total_static_power = (static_power_gather_network + static_power_optical_network + static_power_scatter_network * m_num_scatter_networks_per_cluster);
   }

   // This function is called on all the cores
   volatile double total_dynamic_energy = (m_gather_network_link_model->getDynamicEnergy() + m_optical_network_link_model->getDynamicEnergy() + m_scatter_network_link_model->getDynamicEnergy());

   out << "    Static Power: " << total_static_power << endl;
   out << "    Dynamic Energy: " << total_dynamic_energy << endl;
}
