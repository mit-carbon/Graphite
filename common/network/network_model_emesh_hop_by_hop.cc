#include <math.h>

#include "network_model_emesh_hop_by_hop.h"
#include "core.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "packet_type.h"
// FIXME: This is a hack. No better way of doing this
#include "shmem_msg.h"

NetworkModelEMeshHopByHop::NetworkModelEMeshHopByHop(Network* net):
   NetworkModel(net),
   m_enabled(false),
   m_bytes_sent(0),
   m_total_packets_sent(0),
   m_total_queueing_delay(0),
   m_total_packet_latency(0)
{
   SInt32 total_cores = Config::getSingleton()->getTotalCores();
   m_core_id = getNetwork()->getCore()->getId();

   m_mesh_width = (SInt32) floor (sqrt(total_cores));
   m_mesh_height = (SInt32) ceil (1.0 * total_cores / m_mesh_width);

   assert(total_cores == (m_mesh_width * m_mesh_height));

   std::string queue_model_type = "";
   // Get the Link Bandwidth, Hop Latency and if it has broadcast tree mechanism
   try
   {
      // Link Bandwidth is specified in bits/clock_cycle
      m_link_bandwidth = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/link_bandwidth");
      // Hop Latency is specified in cycles
      m_hop_latency = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_by_hop/hop_latency");

      // Has broadcast tree?
      m_broadcast_tree_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/broadcast_tree_enabled");

      // Queue Model enabled? If no, this degrades into a hop counter model
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/queue_model/enabled");
      queue_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read parameters from the configuration file");
   }

   UInt64 min_processing_time = 1;

   // Initialize the queue models for all the '4' output directions
   for (UInt32 direction = 0; direction < NUM_OUTPUT_DIRECTIONS; direction ++)
   {
      m_queue_models[direction] = NULL;
   }

   if ((m_core_id / m_mesh_width) != 0)
   {
      m_queue_models[DOWN] = QueueModel::create(queue_model_type, min_processing_time);
   }
   if ((m_core_id % m_mesh_width) != 0)
   {
      m_queue_models[LEFT] = QueueModel::create(queue_model_type, min_processing_time);
   }
   if ((m_core_id / m_mesh_width) != (m_mesh_height - 1))
   {
      m_queue_models[UP] = QueueModel::create(queue_model_type, min_processing_time);
   }
   if ((m_core_id % m_mesh_width) != (m_mesh_width - 1))
   {
      m_queue_models[RIGHT] = QueueModel::create(queue_model_type, min_processing_time);
   }
}

NetworkModelEMeshHopByHop::~NetworkModelEMeshHopByHop()
{}

void
NetworkModelEMeshHopByHop::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = ((ShmemMsg*) pkt.data)->getRequester();
   else // Other Packet types
      requester = pkt.sender;
   
   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);

   if (pkt.receiver == NetPacket::BROADCAST)
   {
      if (m_broadcast_tree_enabled)
      {
         // Broadcast tree is enabled
         // Build the broadcast tree
         SInt32 sx, sy, cx, cy;
            
         computePosition(pkt.sender, sx, sy);
         computePosition(m_core_id, cx, cy);

         if (cy >= sy)
            addHop(UP, NetPacket::BROADCAST, computeCoreId(cx,cy+1), pkt.time, pkt.length, nextHops, requester);
         if (cy <= sy)
            addHop(DOWN, NetPacket::BROADCAST, computeCoreId(cx,cy-1), pkt.time, pkt.length, nextHops, requester);
         if (cy == sy)
         {
            if (cx >= sx)
               addHop(RIGHT, NetPacket::BROADCAST, computeCoreId(cx+1,cy), pkt.time, pkt.length, nextHops, requester);
            if (cx <= sx)
               addHop(LEFT, NetPacket::BROADCAST, computeCoreId(cx-1,cy), pkt.time, pkt.length, nextHops, requester);
            if (cx == sx)
               addHop(SELF, m_core_id, m_core_id, pkt.time, pkt.length, nextHops, requester); 
         }
      }
      else
      {
         // Broadcast tree is not enabled
         // Here, broadcast messages are sent as a collection of unicast messages
         LOG_ASSERT_ERROR(pkt.sender == m_core_id,
               "BROADCAST message to be sent at (%i), original sender(%i), Tree not enabled",
               m_core_id, pkt.sender);

         for (core_id_t i = 0; i < (core_id_t) Config::getSingleton()->getTotalCores(); i++)
         {
            // Unicast message to each core
            OutputDirection direction;
            core_id_t next_dest = getNextDest(i, direction);

            addHop(direction, i, next_dest, pkt.time, pkt.length, nextHops, requester);
         }
      }
   }
   else
   {
      // A Unicast packet
      OutputDirection direction;
      core_id_t next_dest = getNextDest(pkt.receiver, direction);

      addHop(direction, pkt.receiver, next_dest, pkt.time, pkt.length, nextHops, requester);
   }
}

void
NetworkModelEMeshHopByHop::addHop(OutputDirection direction, 
      core_id_t final_dest, core_id_t next_dest, 
      UInt64 pkt_time, UInt32 pkt_length, 
      std::vector<Hop>& nextHops, core_id_t requester)
{
   LOG_ASSERT_ERROR((direction == SELF) || ((direction >= 0) && (direction < NUM_OUTPUT_DIRECTIONS)),
         "Invalid Direction(%u)", direction);

   if ((direction == SELF) || m_queue_models[direction])
   {
      Hop h;
      h.final_dest = final_dest;
      h.next_dest = next_dest;

      if (direction == SELF)
         h.time = pkt_time;
      else      
         h.time = pkt_time + computeLatency(direction, pkt_time, pkt_length + sizeof(NetPacket), requester);

      nextHops.push_back(h);
   }
}

void
NetworkModelEMeshHopByHop::computePosition(core_id_t core_id, SInt32 &x, SInt32 &y)
{
   x = core_id % m_mesh_width;
   y = core_id / m_mesh_width;
}

core_id_t
NetworkModelEMeshHopByHop::computeCoreId(SInt32 x, SInt32 y)
{
   return (y * m_mesh_width + x);
}

UInt64
NetworkModelEMeshHopByHop::computeLatency(OutputDirection direction, UInt64 pkt_time, UInt32 pkt_length, core_id_t requester)
{
   LOG_ASSERT_ERROR((direction >= 0) && (direction < NUM_OUTPUT_DIRECTIONS),
         "Invalid Direction(%u)", direction);

   if ( (!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()) )
      return 0;

   UInt64 processing_time = computeProcessingTime(pkt_length);

   UInt64 queue_delay;
   if (m_queue_model_enabled)
   {
      queue_delay = m_queue_models[direction]->computeQueueDelay(pkt_time, processing_time);
   }
   else
   {
      queue_delay = 0;
   }

   UInt64 packet_latency = m_hop_latency + queue_delay + processing_time;

   // Update Counters
   m_bytes_sent += (UInt64) pkt_length;
   m_total_packets_sent ++;
   m_total_queueing_delay += queue_delay;
   m_total_packet_latency += packet_latency;
   
   return packet_latency;
}

UInt64 
NetworkModelEMeshHopByHop::computeProcessingTime(UInt32 pkt_length)
{
   // Send: (pkt_length * 8) bits
   // Bandwidth: (m_link_bandwidth) bits/cycle
   UInt32 num_bits = pkt_length * 8;
   if (num_bits % m_link_bandwidth == 0)
      return (UInt64) (num_bits/m_link_bandwidth);
   else
      return (UInt64) (num_bits/m_link_bandwidth + 1);
}

SInt32
NetworkModelEMeshHopByHop::getNextDest(SInt32 final_dest, OutputDirection& direction)
{
   // Do dimension-order routing
   // Curently, do store-and-forward routing
   // FIXME: Should change this to wormhole routing eventually
   
   SInt32 sx, sy, dx, dy;

   computePosition(m_core_id, sx, sy);
   computePosition(final_dest, dx, dy);

   if (sx > dx)
   {
      direction = LEFT;
      return computeCoreId(sx-1,sy);
   }
   else if (sx < dx)
   {
      direction = RIGHT;
      return computeCoreId(sx+1,sy);
   }
   else if (sy > dy)
   {
      direction = DOWN;
      return computeCoreId(sx,sy-1);
   }
   else if (sy < dy)
   {
      direction = UP;
      return computeCoreId(sx,sy+1);
   }
   else
   {
      // A send to itself
      direction = SELF;
      return m_core_id;
   }
}

void
NetworkModelEMeshHopByHop::outputSummary(std::ostream &out)
{
   out << "    bytes sent: " << m_bytes_sent << std::endl;
   out << "    packets sent: " << m_total_packets_sent << std::endl;
   if (m_total_packets_sent > 0)
   {
      out << "    average queueing delay: " << 
         ((float) m_total_queueing_delay / m_total_packets_sent) << std::endl;
      out << "    average packet latency: " <<
         ((float) m_total_packet_latency / m_total_packets_sent) << std::endl;
   }
   else
   {
      out << "    average queueing delay: " << 
         "NA" << std::endl;
      out << "    average packet latency: " <<
         "NA" << std::endl;
   }
}

void
NetworkModelEMeshHopByHop::enable()
{
   m_enabled = true;
}

void
NetworkModelEMeshHopByHop::disable()
{
   m_enabled = false;
}

bool
NetworkModelEMeshHopByHop::isEnabled()
{
   return m_enabled;
}

std::pair<bool,SInt32>
NetworkModelEMeshHopByHop::computeCoreCountConstraints(SInt32 core_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(core_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * core_count / mesh_width);

   assert(core_count <= mesh_width * mesh_height);
   assert(core_count > (mesh_width - 1) * mesh_height);
   assert(core_count > mesh_width * (mesh_height - 1));

   return std::make_pair(true,mesh_height * mesh_width);
}
