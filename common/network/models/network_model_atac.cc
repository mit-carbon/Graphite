#include <math.h>
#include <vector>
using namespace std;

#include "network_model_atac.h"
#include "simulator.h"
#include "tile_manager.h"
#include "tile.h"
#include "config.h"
#include "log.h"
#include "packet_type.h"
#include "clock_converter.h"
#include "utils.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"
#include "memory_manager_base.h"

UInt32 NetworkModelAtac::m_total_tiles = 0;
SInt32 NetworkModelAtac::m_cluster_size = 0;
SInt32 NetworkModelAtac::m_sqrt_cluster_size = 0;
UInt32 NetworkModelAtac::m_num_clusters = 0;
SInt32 NetworkModelAtac::m_mesh_width = 0;
SInt32 NetworkModelAtac::m_mesh_height = 0;
UInt32 NetworkModelAtac::m_unicast_routing_threshold = 0;

NetworkModelAtac::NetworkModelAtac(Network *net, SInt32 network_id):
   NetworkModel(net, network_id)
{
   m_tile_id = getNetwork()->getTile()->getId();
  
   // Initialize ENet, ONet and BNet parameters
   createANetRouterAndLinkModels();

   // Optical Hub created only on one of the tiles in the cluster
   createOpticalHub();
}

NetworkModelAtac::~NetworkModelAtac()
{
   // Destroy the Optical hub created on one of the tiles in the cluster
   destroyOpticalHub();

   // Destroy the Link Models
   destroyANetRouterAndLinkModels();
}

void
NetworkModelAtac::initializeANetTopologyParams()
{
   // Initialize m_total_tiles, m_cluster_size, m_sqrt_cluster_size, m_mesh_width, m_mesh_height, m_num_clusters
   m_total_tiles = Config::getSingleton()->getTotalTiles();

   try
   {
      m_cluster_size = Sim()->getCfg()->getInt("network/atac/cluster_size");
      m_unicast_routing_threshold = Sim()->getCfg()->getInt("network/atac/unicast_routing_threshold");
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
   m_mesh_width = (SInt32) floor(sqrt(m_total_tiles));
   m_mesh_height = (SInt32) ceil(1.0 * m_total_tiles / m_mesh_width);
   LOG_ASSERT_ERROR(m_mesh_width % m_sqrt_cluster_size == 0, \
         "Mesh Width(%i) must be a multiple of sqrt_cluster_size(%i)", \
         m_mesh_width, m_sqrt_cluster_size);
   LOG_ASSERT_ERROR(m_mesh_height == (m_mesh_width + 1),
         "Mesh Width(%i), Mesh Height(%i)", m_mesh_width, m_mesh_height);
   LOG_ASSERT_ERROR((m_mesh_width * m_mesh_height) == (SInt32) m_total_tiles,
         "Mesh Width(%i), Mesh Height(%i), Tile Count(%i)", m_mesh_width, m_mesh_height, m_total_tiles);

   // Number of Clusters
   m_num_clusters = (m_mesh_width / m_sqrt_cluster_size) * ceil(1.0 * m_mesh_height / m_sqrt_cluster_size);
}

void
NetworkModelAtac::createANetRouterAndLinkModels()
{
   // 1) Initialize Frequency Parameters
   // Everything is normalized to the frequency of the ENet - the frequency of the network
   // is taken as the frequency of the ENet
   try
   {
      m_gather_network_frequency = Sim()->getCfg()->getFloat("network/atac/enet/frequency");
      m_optical_network_frequency = Sim()->getCfg()->getFloat("network/atac/onet/frequency");
      m_scatter_network_frequency = Sim()->getCfg()->getFloat("network/atac/bnet/frequency");
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
      m_gather_network_link_width = Sim()->getCfg()->getInt("network/atac/enet/link/width");
      m_optical_network_link_width = Sim()->getCfg()->getInt("network/atac/onet/link/width");
      m_scatter_network_link_width = Sim()->getCfg()->getInt("network/atac/bnet/link/width");
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
   UInt32 num_flits_per_output_buffer_gather_network_router = 0;
   volatile double waveguide_length;
   volatile double scatter_network_link_length;
   try
   {
      // Router Delay of the electrical mesh network - Specified in clock cycles
      gather_network_router_delay = Sim()->getCfg()->getInt("network/atac/enet/router/delay");
      // Router Power Modeling
      num_flits_per_output_buffer_gather_network_router = Sim()->getCfg()->getInt("network/atac/enet/router/num_flits_per_port_buffer");
      // Length of a gather delay link
      gather_network_link_length = Sim()->getCfg()->getFloat("network/atac/enet/link/length");

      // Waveguide length of the optical network
      waveguide_length = Sim()->getCfg()->getFloat("network/atac/onet/link/length");
     
      // Length of a scatter network link (for the purpose of modeling power) 
      scatter_network_link_length = Sim()->getCfg()->getFloat("network/atac/bnet/link/length");
      // Specified in terms of clock cycles where the clock frequency = scatter_network_frequency
      // Delay is the delay of one scatter_network network. So, you need to pay just this delay when
      // going from the receiver hub to the receiver tile
      m_scatter_network_delay = Sim()->getCfg()->getInt("network/atac/bnet/network_delay");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet link delay and length parameters from the cfg file");
   }

   // 4) Link type parameters
   try
   {
      m_gather_network_link_type = Sim()->getCfg()->getString("network/atac/enet/link/type");
      m_scatter_network_link_type = Sim()->getCfg()->getString("network/atac/bnet/link/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read ANet link type parameters from the cfg file");
   }

   // Optical Network Link Models
   m_optical_network_link_model = new OpticalNetworkLinkModel(m_optical_network_frequency,
         waveguide_length,
         m_optical_network_link_width);
   m_optical_network_link_delay = (UInt64) (ceil( ((double) m_optical_network_link_model->getDelay()) /
                      (m_optical_network_frequency / m_gather_network_frequency) ) );

   // Gather Network Router and Link Models
   m_num_gather_network_router_ports = 5;

   m_gather_network_router_model = ElectricalNetworkRouterModel::create(m_num_gather_network_router_ports, 
         m_num_gather_network_router_ports,
         num_flits_per_output_buffer_gather_network_router, m_gather_network_link_width);
   m_gather_network_link_model = ElectricalNetworkLinkModel::create(m_gather_network_link_type,
         m_gather_network_frequency,
         gather_network_link_length,
         m_gather_network_link_width);
   // Specified in terms of clock cycles where the clock frequency = gather_network frequency
   // Delay is the delay of one gather_network link
   // There may be multiple gather_network links from the tile to the sending hub
   UInt64 gather_network_link_delay = m_gather_network_link_model->getDelay();
   // gather_network_link_delay is already initialized
   m_gather_network_hop_delay = gather_network_link_delay + gather_network_router_delay;

   // Scatter Network link models
   // FIXME: Currently, the BNet network power is modeled using multiple scatter_network links.
   m_scatter_network_link_model = ElectricalNetworkLinkModel::create(m_scatter_network_link_type,
         m_scatter_network_frequency,
         scatter_network_link_length,
         m_scatter_network_link_width);
   // m_scatter_network_delay is already initialized. Conversion needed here
   m_scatter_network_delay = (UInt64) (ceil( ((double) m_scatter_network_delay) /
                      (m_scatter_network_frequency / m_gather_network_frequency) ) );

   initializeEventCounters();
}

void
NetworkModelAtac::initializeEventCounters()
{
   // Initialize Event Counters
   m_gather_network_router_buffer_reads = 0;
   m_gather_network_router_buffer_writes = 0;
   m_gather_network_router_switch_allocator_traversals = 0;
   m_gather_network_router_crossbar_traversals = 0;
   m_gather_network_link_traversals = 0;
   m_optical_network_link_traversals = 0;
   m_scatter_network_link_traversals = 0;
}

void
NetworkModelAtac::destroyANetRouterAndLinkModels()
{
   delete m_gather_network_router_model;
   delete m_gather_network_link_model;
   delete m_optical_network_link_model;
   delete m_scatter_network_link_model;
}

void
NetworkModelAtac::createOpticalHub()
{
   try
   {
      m_num_scatter_networks_per_cluster = Sim()->getCfg()->getInt("network/atac/num_bnets_per_cluster");
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/atac/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/atac/queue_model/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read cluster and queue model parameters from the cfg file");
   }

   createQueueModels();
}

void
NetworkModelAtac::destroyOpticalHub()
{
   destroyQueueModels();
}

void
NetworkModelAtac::createQueueModels()
{
   // Hub Queue Models
   if (m_queue_model_enabled && (m_tile_id == getTileIDWithOpticalHub(getClusterID(m_tile_id))))
   {
      // I am one of the tiles with an optical hub
      UInt64 min_processing_time = 1;
     
      // Create Sending Hub 
      m_sender_hub_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
      m_total_sender_hub_contention_delay = 0;
      m_total_sender_hub_packets = 0;
      m_total_buffered_sender_hub_packets = 0; 

      // Create Receiving Hub 
      m_receiver_hub_queue_models = new QueueModel*[m_num_scatter_networks_per_cluster];
      m_total_receiver_hub_contention_delay = new UInt64[m_num_scatter_networks_per_cluster];
      m_total_receiver_hub_packets = new UInt64[m_num_scatter_networks_per_cluster];
      m_total_buffered_receiver_hub_packets = new UInt64[m_num_scatter_networks_per_cluster];
      
      for (SInt32 i = 0; i < (SInt32) m_num_scatter_networks_per_cluster; i++)
      {
         m_receiver_hub_queue_models[i] = QueueModel::create(m_queue_model_type, min_processing_time);
         m_total_receiver_hub_contention_delay[i] = 0;
         m_total_receiver_hub_packets[i] = 0;
         m_total_buffered_receiver_hub_packets[i] = 0;
      }
   }
   else
   {
      m_sender_hub_queue_model = (QueueModel*) NULL;
      m_receiver_hub_queue_models = (QueueModel**) NULL;
   }

   // Gather Network Queue Models
   if (m_queue_model_enabled)
   {
      for (UInt32 direction = 0; direction < NUM_GATHER_NETWORK_OUTPUT_DIRECTIONS; direction ++)
      {
         UInt64 min_processing_time = 1;
         m_gather_network_queue_models[direction] = QueueModel::create(m_queue_model_type, min_processing_time);
      }
   }
}

void
NetworkModelAtac::destroyQueueModels()
{
   // Hub Queue Models
   if ((m_queue_model_enabled) && (m_tile_id == getTileIDWithOpticalHub(getClusterID(m_tile_id))))
   {
      delete m_sender_hub_queue_model;
      
      for (SInt32 i = 0; i < (SInt32) m_num_scatter_networks_per_cluster; i++)
         delete m_receiver_hub_queue_models[i];
      delete m_receiver_hub_queue_models;

      delete m_total_receiver_hub_contention_delay;
      delete m_total_receiver_hub_packets;
   }

   // Gather Network Queue Models
   if (m_queue_model_enabled)
   {
      for (UInt32 direction = 0; direction < NUM_GATHER_NETWORK_OUTPUT_DIRECTIONS; direction ++)
         delete m_gather_network_queue_models[direction];
   }
}

UInt64 
NetworkModelAtac::computeProcessingTime(UInt32 pkt_length, volatile double bandwidth)
{
   return static_cast<UInt64>(ceil(static_cast<double>(pkt_length * 8) / bandwidth));
}

UInt64
NetworkModelAtac::getHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, SInt32 cluster_id, UInt64 pkt_time, const NetPacket& pkt)
{
   tile_id_t tile_id_with_optical_hub = getTileIDWithOpticalHub(cluster_id);
   Tile* tile = Sim()->getTileManager()->getTileFromID(tile_id_with_optical_hub);
   assert(tile);
   NetworkModelAtac* network_model = (NetworkModelAtac*) tile->getNetwork()->getNetworkModelFromPacketType(pkt.type);

   return network_model->computeHubQueueDelay(hub_type, sender_cluster_id, pkt_time, pkt);
}

UInt64
NetworkModelAtac::computeHubQueueDelay(NetworkComponentType hub_type, SInt32 sender_cluster_id, UInt64 pkt_time, const NetPacket& pkt)
{
   tile_id_t requester = getRequester(pkt);
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   if ( (!isEnabled()) ||
        (!m_queue_model_enabled) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (!getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) )
      return 0;

   assert(m_tile_id == getTileIDWithOpticalHub(getClusterID(m_tile_id)));
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

            LOG_ASSERT_ERROR(sender_cluster_id == getClusterID(m_tile_id),
                  "sender_cluster_id(%i), curr_cluster_id(%i)",
                  sender_cluster_id, getClusterID(m_tile_id));
            UInt64 sender_hub_queue_delay = m_sender_hub_queue_model->computeQueueDelay(optical_network_pkt_time, processing_time);

            // Performance Counters
            m_total_sender_hub_contention_delay += sender_hub_queue_delay;
            if (sender_hub_queue_delay != 0)
               m_total_buffered_sender_hub_packets ++;
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
            if (receiver_hub_queue_delay != 0)
               m_total_buffered_receiver_hub_packets[scatter_network_num] ++;
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
NetworkModelAtac::computeAction(const NetPacket& pkt)
{
   assert(pkt.specific == RECEIVER_TILE);
   LOG_ASSERT_ERROR((TILE_ID(pkt.receiver) == NetPacket::BROADCAST) || (TILE_ID(pkt.receiver) == m_tile_id),
                    "pkt.receiver(%i)", pkt.receiver);
   return RoutingAction::RECEIVE;
}

UInt64
NetworkModelAtac::routePacketOnGatherNetwork(const NetPacket &pkt, tile_id_t next_receiver)
{
   // Release Lock
   m_lock.release();

   // Return Latency
   assert(TILE_ID(pkt.sender) == m_tile_id);
   tile_id_t requester = getRequester(pkt);
   if ( (!isEnabled()) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (!getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) )
   {
      // Acquire Lock back again
      m_lock.acquire();

      return 0;
   }

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   tile_id_t curr_tile_id = m_tile_id;
   UInt64 curr_time = pkt.time;
   do
   {
      // Get the remote network model
      Tile* tile = Sim()->getTileManager()->getTileFromID(curr_tile_id);
      assert(tile);
      NetworkModelAtac* model = (NetworkModelAtac*) tile->getNetwork()->getNetworkModelFromPacketType(pkt.type);

      // Do Routing
      pair<tile_id_t,UInt64> next_hop = model->__routePacketOnGatherNetwork(pkt, curr_time, pkt_length, next_receiver);

      // Update Variables
      curr_tile_id = next_hop.first;
      curr_time = next_hop.second;
   } while (curr_tile_id != next_receiver);

   // Acquire Lock back again
   m_lock.acquire();

   assert(curr_time >= pkt.time);
   return (curr_time - pkt.time);
}

void 
NetworkModelAtac::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   // Lock
   m_lock.acquire();

   assert(Config::getSingleton()->getProcessCount() == 1);
   assert(m_tile_id == TILE_ID(pkt.sender));

   // All the energies and delays are computed at source
   if (TILE_ID(pkt.receiver) == NetPacket::BROADCAST)
   {
      UInt64 gather_network_delay = routePacketOnGatherNetwork(pkt, getTileIDWithOpticalHub(getClusterID(m_tile_id)));

      updateDynamicEnergy(OPTICAL_NETWORK, pkt);
      
      UInt64 sender_hub_queue_delay = getHubQueueDelay(SENDER_HUB,
            getClusterID(pkt.sender),
            getClusterID(pkt.sender),
            pkt.time + gather_network_delay,
            pkt);
      UInt64 latency_sender_tile_to_receiver_hub = gather_network_delay + 
                                                   sender_hub_queue_delay + 
                                                   m_optical_network_link_delay;

      UInt64 receiver_hub_queue_delay[m_num_clusters];
      for (SInt32 i = 0; i < (SInt32) m_num_clusters; i++)
      {
         updateDynamicEnergy(SCATTER_NETWORK, pkt);
         
         receiver_hub_queue_delay[i] = getHubQueueDelay(RECEIVER_HUB,
               getClusterID(pkt.sender),
               i,
               pkt.time + latency_sender_tile_to_receiver_hub,
               pkt);
      }

      for (tile_id_t i = 0; i < (tile_id_t) m_total_tiles; i++)
      {
         UInt64 latency_receiver_hub_to_receiver_tile = receiver_hub_queue_delay[getClusterID(i)] +
                                                        m_scatter_network_delay;

         UInt64 total_latency = latency_sender_tile_to_receiver_hub +
                                latency_receiver_hub_to_receiver_tile;

         Hop h;
         h.next_dest = CORE_ID(i);
         h.final_dest = CORE_ID(NetPacket::BROADCAST);
         h.specific = RECEIVER_TILE;
         h.time = pkt.time + total_latency;
         
         nextHops.push_back(h);
      }
   }
   else // if (pkt.receiver != NetPacket::BROADCAST)
   {
      // Right now, it is a unicast/multicast
      LOG_ASSERT_ERROR(TILE_ID(pkt.receiver) < (tile_id_t) m_total_tiles,
            "Got invalid receiver ID = %i", pkt.receiver);

      UInt64 total_latency;
      if (computeGlobalRoute(pkt.sender, pkt.receiver) == GLOBAL_ENET)
      {
         // Router on ENet
         total_latency = routePacketOnGatherNetwork(pkt, TILE_ID(pkt.receiver));
      }
      else
      {
         // Route on ANet
         UInt64 gather_network_delay = routePacketOnGatherNetwork(pkt, getTileIDWithOpticalHub(getClusterID(m_tile_id)));

         UInt64 latency_sender_tile_to_receiver_hub;

         // Update Dynamic Energy Counters         
         updateDynamicEnergy(OPTICAL_NETWORK, pkt);
         
         UInt64 sender_hub_queue_delay = getHubQueueDelay(SENDER_HUB,
               getClusterID(pkt.sender),
               getClusterID(pkt.sender),
               pkt.time + gather_network_delay,
               pkt);
         latency_sender_tile_to_receiver_hub = gather_network_delay +
                                               sender_hub_queue_delay +
                                               m_optical_network_link_delay;

         // Update Dynamic Energy Counters
         updateDynamicEnergy(SCATTER_NETWORK, pkt);

         UInt64 receiver_hub_queue_delay = getHubQueueDelay(RECEIVER_HUB,
               getClusterID(pkt.sender),
               getClusterID(pkt.receiver),
               pkt.time + latency_sender_tile_to_receiver_hub,
               pkt);
         UInt64 latency_receiver_hub_to_receiver_tile = receiver_hub_queue_delay +
                                                        m_scatter_network_delay;

         total_latency = latency_sender_tile_to_receiver_hub +
                         latency_receiver_hub_to_receiver_tile;
      }

      Hop h;
      h.next_dest = pkt.receiver;
      h.final_dest = pkt.receiver;
      h.specific = RECEIVER_TILE;
      h.time = pkt.time + total_latency;
      
      nextHops.push_back(h);
   }

   m_lock.release();
}

void
NetworkModelAtac::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);
 
   tile_id_t requester = getRequester(pkt);
   if ( (!isEnabled()) || 
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (!getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) )
      return;

   // Compute Processing Time
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   UInt64 processing_time = computeProcessingTime(pkt_length, m_scatter_network_link_width); 
   
   // Add Serialization Delay
   if (TILE_ID(pkt.sender) != TILE_ID(pkt.receiver))
      pkt.time += processing_time;

   // Compute Zero Load Latency
   UInt64 zero_load_latency;  
   if (TILE_ID(pkt.sender) == TILE_ID(pkt.receiver))
   {
      zero_load_latency = 0;
   }
   else if (computeGlobalRoute(pkt.sender, pkt.receiver) == GLOBAL_ANET)
   {
      UInt64 gather_network_delay = computeNumHopsOnGatherNetwork(TILE_ID(pkt.sender), getTileIDWithOpticalHub(getClusterID(pkt.sender))) * 
                                    m_gather_network_hop_delay;
      zero_load_latency = gather_network_delay + m_optical_network_link_delay + m_scatter_network_delay + processing_time;
   }
   else if (computeGlobalRoute(pkt.sender, pkt.receiver) == GLOBAL_ENET)
   {
      zero_load_latency = computeNumHopsOnGatherNetwork(TILE_ID(pkt.sender), TILE_ID(pkt.receiver)) * m_gather_network_hop_delay;
   }

   // Update Receive Counters
   updateReceiveCounters(pkt, zero_load_latency);
}

NetworkModelAtac::GlobalRoute
NetworkModelAtac::computeGlobalRoute(core_id_t sender, core_id_t receiver)
{
   if (TILE_ID(receiver) == NetPacket::BROADCAST)
      return GLOBAL_ANET;

   UInt32 num_hops_on_gather_network = computeNumHopsOnGatherNetwork(TILE_ID(sender), TILE_ID(receiver));
   return (num_hops_on_gather_network <= m_unicast_routing_threshold) ? GLOBAL_ENET : GLOBAL_ANET;
}

void
NetworkModelAtac::outputHubSummary(ostream& out)
{
   out << " ATAC Cluster: " << endl;
   if ((m_queue_model_enabled) && (m_tile_id == getTileIDWithOpticalHub(getClusterID(m_tile_id))))
   {
      // ONet
      if (m_total_sender_hub_packets > 0)
      {
         // Convert from optical network clock to global clock
         UInt64 total_sender_hub_contention_delay_in_ns = convertCycleCount(
               m_total_sender_hub_contention_delay,
               m_optical_network_frequency, 1.0);
         out << "    ONet Link Contention Delay (in ns): "
             << ((float) total_sender_hub_contention_delay_in_ns) / m_total_sender_hub_packets
             << endl;
      }
      else
      {
         out << "    ONet Link Contention Delay (in ns): 0" << endl;
      }
      out << "    ONet Total Packets: " << m_total_sender_hub_packets << endl;
      out << "    ONet Total Buffered Packets: " << m_total_buffered_sender_hub_packets << endl;
    
      if ((m_queue_model_type == "history_list") || (m_queue_model_type == "history_tree"))
      {
         float sender_hub_utilization;
         double frac_analytical_model_used;
         if (m_queue_model_type == "history_list")
         {
            sender_hub_utilization = ((QueueModelHistoryList*) m_sender_hub_queue_model)->getQueueUtilization();
            frac_analytical_model_used = ((double) ((QueueModelHistoryList*) m_sender_hub_queue_model)->getTotalRequestsUsingAnalyticalModel()) / ((QueueModelHistoryList*) m_sender_hub_queue_model)->getTotalRequests();
         }
         else // (m_queue_model_type == "history_tree")
         {
            sender_hub_utilization = ((QueueModelHistoryTree*) m_sender_hub_queue_model)->getQueueUtilization();
            frac_analytical_model_used = ((double) ((QueueModelHistoryTree*) m_sender_hub_queue_model)->getTotalRequestsUsingAnalyticalModel()) / ((QueueModelHistoryTree*) m_sender_hub_queue_model)->getTotalRequests();
         }
         out << "    ONet Link Utilization(\%): " << sender_hub_utilization * 100 << endl; 
         out << "    ONet Analytical Model Used(\%): " << frac_analytical_model_used * 100 << endl;
      }

      // BNet
      for (UInt32 i = 0; i < m_num_scatter_networks_per_cluster; i++)
      {
         if (m_total_receiver_hub_packets[i] > 0)
         {
            // Convert from scatter network clock to global clock
            UInt64 total_receiver_hub_contention_delay_in_ns = convertCycleCount(
                  m_total_receiver_hub_contention_delay[i],
                  m_scatter_network_frequency, 1.0);
            out << "    BNet (" << i << ") Link Contention Delay (in ns): "
                << ((float) total_receiver_hub_contention_delay_in_ns) / m_total_receiver_hub_packets[i]
                << endl;
         }
         else
         {
            out << "    BNet (" << i << ") Link Contention Delay (in ns): 0" << endl;
         }
         out << "    BNet (" << i << ") Total Packets: " << m_total_receiver_hub_packets[i] << endl;
         out << "    BNet (" << i << ") Total Buffered Packets: " << m_total_buffered_receiver_hub_packets[i] << endl;

         if ((m_queue_model_type == "history_list") || (m_queue_model_type == "history_tree"))
         {
            float receiver_hub_utilization;
            double frac_analytical_model_used;
            if (m_queue_model_type == "history_list")
            {
               receiver_hub_utilization = ((QueueModelHistoryList*) m_receiver_hub_queue_models[i])->getQueueUtilization();
               frac_analytical_model_used = ((double) ((QueueModelHistoryList*) m_receiver_hub_queue_models[i])->getTotalRequestsUsingAnalyticalModel()) / ((QueueModelHistoryList*) m_receiver_hub_queue_models[i])->getTotalRequests();
            }
            else // (m_queue_model_type == "history_tree")
            {
               receiver_hub_utilization = ((QueueModelHistoryTree*) m_receiver_hub_queue_models[i])->getQueueUtilization();
               frac_analytical_model_used = ((double) ((QueueModelHistoryTree*) m_receiver_hub_queue_models[i])->getTotalRequestsUsingAnalyticalModel()) / ((QueueModelHistoryTree*) m_receiver_hub_queue_models[i])->getTotalRequests();
            }
            out << "    BNet (" << i << ") Link Utilization(\%): " << receiver_hub_utilization * 100 << endl;
            out << "    BNet (" << i << ") Analytical Model Used(\%): " << frac_analytical_model_used * 100 << endl;
         }
      }
   }
   else
   {
      out << "    ONet Link Contention Delay (in ns): NA" << endl;
      out << "    ONet Total Packets: NA" << endl;
      out << "    ONet Total Buffered Packets: NA" << endl;
      if ((m_queue_model_type == "history_list") || (m_queue_model_type == "history_tree"))
      {
         out << "    ONet Link Utilization(\%): NA" << endl;
         out << "    ONet Analytical Model Used(\%): NA" << endl;
      }
      
      for (UInt32 i = 0; i < m_num_scatter_networks_per_cluster; i++)
      {
         out << "    BNet (" << i << ") Link Contention Delay (in ns): NA" << endl;
         out << "    BNet (" << i << ") Total Packets: NA" << endl;
         out << "    BNet (" << i << ") Total Buffered Packets: NA" << endl;
         if ((m_queue_model_type == "history_list") || (m_queue_model_type == "history_tree"))
         {
            out << "    BNet (" << i << ") Link Utilization(\%): NA" << endl;
            out << "    BNet (" << i << ") Analytical Model Used(\%): NA" << endl;
         }
      }
   }
}

void
NetworkModelAtac::outputSummary(ostream &out)
{
   NetworkModel::outputSummary(out);

   outputHubSummary(out);
   outputPowerSummary(out);
}

SInt32
NetworkModelAtac::getClusterID(core_id_t core_id)
{
   return getClusterID(core_id.tile_id);
}

SInt32
NetworkModelAtac::getClusterID(tile_id_t tile_id)
{
   // Consider a mesh formed by the clusters
   SInt32 cluster_mesh_width;
   cluster_mesh_width = m_mesh_width / m_sqrt_cluster_size;

   SInt32 tile_x, tile_y;
   tile_x = tile_id % m_mesh_width;
   tile_y = tile_id / m_mesh_width;

   SInt32 cluster_pos_x, cluster_pos_y;
   cluster_pos_x = tile_x / m_sqrt_cluster_size;
   cluster_pos_y = tile_y / m_sqrt_cluster_size;

   return (cluster_pos_y * cluster_mesh_width + cluster_pos_x);
}

tile_id_t
NetworkModelAtac::getTileIDWithOpticalHub(SInt32 cluster_id)
{
   // Consider a mesh formed by the clusters
   SInt32 cluster_mesh_width;
   cluster_mesh_width = m_mesh_width / m_sqrt_cluster_size;

   SInt32 cluster_pos_x, cluster_pos_y;
   cluster_pos_x = (cluster_id % cluster_mesh_width);
   cluster_pos_y = (cluster_id / cluster_mesh_width);

   SInt32 optical_hub_x, optical_hub_y; 
   optical_hub_x = (cluster_pos_x * m_sqrt_cluster_size) + (m_sqrt_cluster_size-1)/2;
   optical_hub_y = (cluster_pos_y * m_sqrt_cluster_size) + (m_sqrt_cluster_size-1)/2;
   
   SInt32 tile_x, tile_y;
   tile_x = (optical_hub_x >= m_mesh_width) ? (optical_hub_x - (m_sqrt_cluster_size-1)/2) : optical_hub_x;
   tile_y = (optical_hub_y >= m_mesh_height) ? (optical_hub_y - (m_sqrt_cluster_size-1)/2) : optical_hub_y;

   return (tile_y * m_mesh_width + tile_x);
}

void
NetworkModelAtac::getTileIDListInCluster(SInt32 cluster_id, vector<tile_id_t>& tile_id_list)
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
         SInt32 tile_id = j * m_mesh_width + i;
         if (tile_id < (SInt32) Config::getSingleton()->getTotalTiles())
            tile_id_list.push_back(tile_id);
      }
   }
}

UInt32
NetworkModelAtac::computeNumHopsOnGatherNetwork(tile_id_t sender, tile_id_t receiver)
{
   SInt32 sx, sy, dx, dy;
   computePositionOnGatherNetwork(sender, sx, sy);
   computePositionOnGatherNetwork(receiver, dx, dy);
   return (abs(sx-dx) + abs(sy-dy));
}

void
NetworkModelAtac::computePositionOnGatherNetwork(tile_id_t tile_id, SInt32& x, SInt32& y)
{
   x = tile_id % m_mesh_width;
   y = tile_id / m_mesh_width;
}

tile_id_t
NetworkModelAtac::computeTileIdOnGatherNetwork(SInt32 x, SInt32 y)
{
   return (y * m_mesh_width + x);
}

UInt64
NetworkModelAtac::computeLatencyOnGatherNetwork(GatherNetworkOutputDirection direction, UInt64 pkt_time, UInt32 pkt_length)
{
   LOG_ASSERT_ERROR((direction == SELF) || ((direction >= 0) && (direction < NUM_GATHER_NETWORK_OUTPUT_DIRECTIONS)),
         "Invalid Direction(%u)", direction);

   if (direction == SELF)
      return 0;
      
   UInt64 processing_time = computeProcessingTime(pkt_length, m_gather_network_link_width);
   // Calculate the contention delay
   UInt64 queue_delay = 0;
   if (m_queue_model_enabled)
      queue_delay = m_gather_network_queue_models[direction]->computeQueueDelay(pkt_time, processing_time);

   return queue_delay + m_gather_network_hop_delay;
}

tile_id_t
NetworkModelAtac::getNextDestOnGatherNetwork(tile_id_t next_receiver, GatherNetworkOutputDirection& direction)
{
   // Do dimension-order routing
   // Curently, do store-and-forward routing
   
   SInt32 sx, sy, dx, dy;

   computePositionOnGatherNetwork(m_tile_id, sx, sy);
   computePositionOnGatherNetwork(next_receiver, dx, dy);

   if (sx > dx)
   {
      direction = LEFT;
      return computeTileIdOnGatherNetwork(sx-1,sy);
   }
   else if (sx < dx)
   {
      direction = RIGHT;
      return computeTileIdOnGatherNetwork(sx+1,sy);
   }
   else if (sy > dy)
   {
      direction = DOWN;
      return computeTileIdOnGatherNetwork(sx,sy-1);
   }
   else if (sy < dy)
   {
      direction = UP;
      return computeTileIdOnGatherNetwork(sx,sy+1);
   }
   else
   {
      // A send to itself
      direction = SELF;
      return m_tile_id;
   }
}

pair<tile_id_t,UInt64>
NetworkModelAtac::__routePacketOnGatherNetwork(const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length, tile_id_t next_receiver)
{
   ScopedLock sl(m_lock);
   
   GatherNetworkOutputDirection direction;
   tile_id_t next_dest = getNextDestOnGatherNetwork(next_receiver, direction);
   UInt64 latency = computeLatencyOnGatherNetwork(direction, pkt_time, pkt_length);
   
   // Update Energy Counters
   updateDynamicEnergy(GATHER_NETWORK, pkt);

   return make_pair(next_dest, pkt_time + latency);
}

pair<bool,SInt32>
NetworkModelAtac::computeTileCountConstraints(SInt32 tile_count)
{
   // Same Calculations as Electrical Mesh Model
   SInt32 mesh_width = (SInt32) floor (sqrt(tile_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * tile_count / mesh_width);

   assert(tile_count <= mesh_width * mesh_height);
   assert(tile_count > (mesh_width - 1) * mesh_height);
   assert(tile_count > mesh_width * (mesh_height - 1));

   return make_pair(true,mesh_height * mesh_width);
}

pair<bool, vector<tile_id_t> >
NetworkModelAtac::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count)
{
   // Initialize the topology parameters in case called by an external model
   // Initialize m_total_tiles, m_cluster_size, m_sqrt_cluster_size, m_mesh_width, m_mesh_height, m_num_clusters
   initializeANetTopologyParams();

   // Initialization should be done by now
   LOG_ASSERT_ERROR(num_memory_controllers <= (SInt32) m_num_clusters,
         "num_memory_controllers(%i), num_clusters(%u)", num_memory_controllers, m_num_clusters);

   vector<tile_id_t> tile_id_list_with_memory_controllers;
   for (SInt32 i = 0; i < num_memory_controllers; i++)
   {
      tile_id_list_with_memory_controllers.push_back(getTileIDWithOpticalHub(i));
   }

   return (make_pair(true, tile_id_list_with_memory_controllers));
}

pair<bool, vector<Config::TileList> >
NetworkModelAtac::computeProcessToTileMapping()
{
   // Initialize m_total_tiles, m_cluster_size, m_sqrt_cluster_size, m_mesh_width, m_mesh_height, m_num_clusters
   initializeANetTopologyParams();

   UInt32 process_count = Config::getSingleton()->getProcessCount();
   vector<Config::TileList> process_to_tile_mapping(process_count);
  
   LOG_ASSERT_WARNING(m_num_clusters >= process_count,
        "Number of Clusters(%u) < Total Process in Simulation(%u)",
        m_num_clusters, process_count);
        
   UInt32 process_num = 0;
   for (UInt32 i = 0; i < m_num_clusters; i++)
   {
      Config::TileList tile_id_list;
      Config::TileList::iterator tile_it;
      getTileIDListInCluster(i, tile_id_list);
      for (tile_it = tile_id_list.begin(); tile_it != tile_id_list.end(); tile_it ++)
      {
         process_to_tile_mapping[process_num].push_back(*tile_it);
      }
      process_num = (process_num + 1) % process_count;
   }

   return (make_pair(true, process_to_tile_mapping));
}

void
NetworkModelAtac::updateDynamicEnergy(SubNetworkType sub_net_type, const NetPacket& pkt)
{
   // This function calls the power models as well as update power counters
   tile_id_t requester = getRequester(pkt);
   if ( (!isEnabled()) ||
        (requester >= ((tile_id_t) Config::getSingleton()->getApplicationTiles())) ||
        (!getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) )
      return;

   // TODO: Make these models more detailed later - Compute the exact number of bit flips
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   switch(sub_net_type)
   {
      case GATHER_NETWORK:
         {
            UInt32 num_flits = computeProcessingTime(pkt_length, m_gather_network_link_width);

            if (Config::getSingleton()->getEnablePowerModeling())
            {
               // Buffer Write Energy
               m_gather_network_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE,
                                                                        m_gather_network_link_width/2, num_flits);
               m_gather_network_router_model->updateDynamicEnergyClock(num_flits);
          
               // Buffer Read Energy
               m_gather_network_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ,
                                                                        m_gather_network_link_width/2, num_flits);
               m_gather_network_router_model->updateDynamicEnergyClock(num_flits);
               
               // Update Dynamic Energy of router (switch allocator + crossbar)
               m_gather_network_router_model->updateDynamicEnergySwitchAllocator(m_num_gather_network_router_ports/2);
               m_gather_network_router_model->updateDynamicEnergyClock(1);

               m_gather_network_router_model->updateDynamicEnergyCrossbar(m_gather_network_link_width/2, num_flits);
               m_gather_network_router_model->updateDynamicEnergyClock(num_flits);
               // We assume that there is no buffering here - so dont update dynamic energy of buffer

               // Update Dynamic Energy of link
               m_gather_network_link_model->updateDynamicEnergy(m_gather_network_link_width / 2, num_flits);
            }

            // Event Counters
            // Router
            m_gather_network_router_buffer_reads += num_flits;
            m_gather_network_router_buffer_writes += num_flits;
            m_gather_network_router_switch_allocator_traversals += 1;
            m_gather_network_router_crossbar_traversals += num_flits;
            
            // Link      
            m_gather_network_link_traversals += num_flits;
         }
         break;

      case OPTICAL_NETWORK:
         {
            UInt32 num_flits = computeProcessingTime(pkt_length, m_optical_network_link_width);
            
            if (Config::getSingleton()->getEnablePowerModeling())
            {
               m_optical_network_link_model->updateDynamicEnergy(m_optical_network_link_width / 2, num_flits);
            }
            
            // Event Counters
            m_optical_network_link_traversals += num_flits;
         }
         break;

      case SCATTER_NETWORK:
         {
            UInt32 num_flits = computeProcessingTime(pkt_length, m_scatter_network_link_width);
            
            // We look at a tree and compute the number of hops in the tree
            if (Config::getSingleton()->getEnablePowerModeling())
            {
               m_scatter_network_link_model->updateDynamicEnergy(m_scatter_network_link_width / 2, num_flits * m_cluster_size);
            }
            
            // Event Counters
            m_scatter_network_link_traversals += num_flits;
         }
         break;

      default:
         LOG_PRINT_ERROR("Unrecognized Sub Network Type(%u)", sub_net_type);
         break;
   }
}

void
NetworkModelAtac::outputPowerSummary(ostream& out)
{
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      volatile double total_static_power = 0.0;
      if (m_tile_id == getTileIDWithOpticalHub(getClusterID(m_tile_id)))
      {
         // I have an optical Hub
         // m_cluster_size is the number of tiles in the cluster + 1 Hub
         // So, a total of (m_cluster_size + 1) nodes
         // We need m_cluster_size edges to connect them to form a tree

         // Get the static power for the entire network.
         // This function will only be called only on Tile0

         // Gather network is modeled as an electrical mesh
         volatile double static_power_gather_network = m_cluster_size * (m_gather_network_router_model->getTotalStaticPower() +
               m_gather_network_link_model->getStaticPower() * (m_num_gather_network_router_ports-1));

         volatile double static_power_optical_network = m_optical_network_link_model->getStaticPower();

         // Scatter network is modeled as an H-tree
         UInt32 total_scatter_network_links = m_cluster_size;
         volatile double static_power_scatter_network = m_scatter_network_link_model->getStaticPower() * total_scatter_network_links;

         total_static_power = ( static_power_gather_network +
                                static_power_optical_network + 
                                (static_power_scatter_network * m_num_scatter_networks_per_cluster) );
      }

      // This function is called on all the tiles
      volatile double total_dynamic_energy = m_gather_network_router_model->getTotalDynamicEnergy() +
                                             m_gather_network_link_model->getDynamicEnergy() +
                                             m_optical_network_link_model->getDynamicEnergy() +
                                             m_scatter_network_link_model->getDynamicEnergy();

      out << "    Static Power: " << total_static_power << endl;
      out << "    Dynamic Energy: " << total_dynamic_energy << endl;
   }

   out << "  Event Counters:" << endl;
   out << "    ENet Router Buffer Reads: " << m_gather_network_router_buffer_reads << endl;
   out << "    ENet Router Buffer Writes: " << m_gather_network_router_buffer_writes << endl;
   out << "    ENet Router Switch Allocator Traversals: " << m_gather_network_router_switch_allocator_traversals << endl;
   out << "    ENet Router Crossbar Traversals: " << m_gather_network_router_crossbar_traversals << endl;
   out << "    ENet Link Traversals: " << m_gather_network_link_traversals << endl;
   out << "    ONet Link Traversals: " << m_optical_network_link_traversals << endl;
   out << "    BNet Link Traversals: " << m_scatter_network_link_traversals << endl;
}
