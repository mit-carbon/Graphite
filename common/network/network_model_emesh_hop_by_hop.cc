#include <math.h>

#include "network_model_emesh_hop_by_hop.h"
#include "core.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"

NetworkModelEMeshHopByHop::NetworkModelEMeshHopByHop(Network* net):
   NetworkModel(net),
   m_enabled(false),
   m_bytes_sent(0),
   m_total_packets_sent(0),
   m_total_queueing_delay(0.0),
   m_total_packet_latency(0.0)
{
   SInt32 total_cores = Config::getSingleton()->getTotalCores();
   m_core_id = getNetwork()->getCore()->getId();

   m_mesh_width = (SInt32) floor (sqrt(total_cores));
   m_mesh_height = (SInt32) ceil (1.0 * total_cores / m_mesh_width);

   assert(total_cores <= m_mesh_width * m_mesh_height);
   assert(total_cores > (m_mesh_width - 1) * m_mesh_height);
   assert(total_cores > m_mesh_width * (m_mesh_height - 1));

   float bisection_bandwidth = 0.0;
   float core_frequency = 0.0;
   float hop_time = 0.0;
   
   bool moving_avg_enabled = false;
   UInt32 moving_avg_window_size = 0;
   std::string moving_avg_type = "";

   // Get the Link Bandwidth, Hop Latency and if it has broadcast tree mechanism
   try
   {
      // Bisection Bandwidth is specified in GB/s
      bisection_bandwidth = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop/bisection_bandwidth");
      // Core Frequency is specified in GHz
      core_frequency = Sim()->getCfg()->getFloat("perf_model/core/frequency");
      // Hop Latency is specified in 'ns'
      hop_time = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop/hop_latency");

      // Has broadcast tree?
      m_broadcast_tree_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/broadcast_tree_enabled");

      // Queue Model enabled? If no, this degrades into a hop counter model
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/queue_model/enabled");

      moving_avg_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/queue_model/moving_avg_enabled");
      moving_avg_window_size = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/queue_model/moving_avg_window_size");
      moving_avg_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/queue_model/moving_avg_type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read parameters from the configuration file");
   }

   // Link Bandwidth in 'bytes/clock cycle'
   m_link_bandwidth = (bisection_bandwidth / core_frequency) / (2 * getMax<SInt32>(m_mesh_width,m_mesh_height));
   // Hop Latency in 'cycles'
   m_hop_latency = (UInt64) (hop_time * core_frequency);

   // Initialize the queue models for all the '4' output directions
   for (UInt32 direction = 0; direction < NUM_OUTPUT_DIRECTIONS; direction ++)
   {
      m_queue_models[direction] = NULL;
   }

   if ((m_core_id / m_mesh_width) != 0)
   {
      m_queue_models[DOWN] = new QueueModel(moving_avg_enabled, moving_avg_window_size, moving_avg_type);
   }
   if ((m_core_id % m_mesh_width) != 0)
   {
      m_queue_models[LEFT] = new QueueModel(moving_avg_enabled, moving_avg_window_size, moving_avg_type);
   }
   if ((m_core_id / m_mesh_width) != (m_mesh_height - 1))
   {
      m_queue_models[UP] = new QueueModel(moving_avg_enabled, moving_avg_window_size, moving_avg_type);
   }
   if ((m_core_id % m_mesh_width) != (m_mesh_width - 1))
   {
      m_queue_models[RIGHT] = new QueueModel(moving_avg_enabled, moving_avg_window_size, moving_avg_type);
   }
}

NetworkModelEMeshHopByHop::~NetworkModelEMeshHopByHop()
{}

void
NetworkModelEMeshHopByHop::routePacket(const NetPacket &pkt, std::vector<Hop> &nextHops)
{
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
            addHop(UP, NetPacket::BROADCAST, computeCoreId(cx,cy+1), pkt.time, pkt.length, nextHops);
         if (cy <= sy)
            addHop(DOWN, NetPacket::BROADCAST, computeCoreId(cx,cy-1), pkt.time, pkt.length, nextHops);
         if (cy == sy)
         {
            if (cx >= sx)
               addHop(RIGHT, NetPacket::BROADCAST, computeCoreId(cx+1,cy), pkt.time, pkt.length, nextHops);
            if (cx <= sx)
               addHop(LEFT, NetPacket::BROADCAST, computeCoreId(cx-1,cy), pkt.time, pkt.length, nextHops);
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

            addHop(direction, i, next_dest, pkt.time, pkt.length, nextHops);
         }
      }
   }
   else
   {
      // A Unicast packet
      OutputDirection direction;
      core_id_t next_dest = getNextDest(pkt.receiver, direction);

      addHop(direction, pkt.receiver, next_dest, pkt.time, pkt.length, nextHops);
   }
}

void
NetworkModelEMeshHopByHop::addHop(OutputDirection direction, 
      core_id_t final_dest, core_id_t next_dest, 
      UInt64 pkt_time, UInt64 pkt_length, 
      std::vector<Hop>& nextHops)
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
         h.time = pkt_time + computeLatency(direction, pkt_time, pkt_length + sizeof(NetPacket));

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
NetworkModelEMeshHopByHop::computeLatency(OutputDirection direction, UInt64 pkt_time, UInt64 pkt_size)
{
   LOG_ASSERT_ERROR((direction >= 0) && (direction < NUM_OUTPUT_DIRECTIONS),
         "Invalid Direction(%u)", direction);

   if (!m_enabled)
      return 0;

   UInt64 processing_time = (UInt64) ((float) pkt_size/m_link_bandwidth) + 1;

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
   m_bytes_sent += pkt_size;
   m_total_packets_sent ++;
   m_total_queueing_delay += (double) queue_delay;
   m_total_packet_latency += (double) packet_latency;
   
   return packet_latency;
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
   out << "    average queueing delay: " << 
      (float) (m_total_queueing_delay / m_total_packets_sent) << std::endl;
   out << "    average packet latency: " <<
      (float) (m_total_packet_latency / m_total_packets_sent) << std::endl;
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
