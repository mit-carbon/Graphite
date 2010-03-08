#include <math.h>
using namespace std;

#include "network_model_emesh_hop_by_hop.h"
#include "core.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "packet_type.h"
// FIXME: This is a hack. No better way of doing this
#include "shmem_msg.h"
#include "queue_model_history_list.h"

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

   string queue_model_type = "";
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
{
   for (UInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
   {
      if (m_queue_models[i])
         delete m_queue_models[i];
   }
}

void
NetworkModelEMeshHopByHop::routePacket(const NetPacket &pkt, vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

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
      vector<Hop>& nextHops, core_id_t requester)
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
NetworkModelEMeshHopByHop::outputSummary(ostream &out)
{
   out << "    bytes sent: " << m_bytes_sent << endl;
   out << "    packets sent: " << m_total_packets_sent << endl;
   if (m_total_packets_sent > 0)
   {
      out << "    average queueing delay: " << 
         ((float) m_total_queueing_delay / m_total_packets_sent) << endl;
      out << "    average packet latency: " <<
         ((float) m_total_packet_latency / m_total_packets_sent) << endl;
   }
   else
   {
      out << "    average queueing delay: " << 
         "NA" << endl;
      out << "    average packet latency: " <<
         "NA" << endl;
   }

   string queue_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/queue_model/type");
   if (m_queue_model_enabled && (queue_model_type == "history_list"))
   {
      out << "  Queue Models:" << endl;
         
      float queue_utilization = 0.0;
      float frac_requests_using_analytical_model = 0.0;
      UInt32 num_queue_models = 0;
      
      for (SInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
      {
         if (m_queue_models[i])
         {
            queue_utilization += ((QueueModelHistoryList*) m_queue_models[i])->getQueueUtilization();
            frac_requests_using_analytical_model += ((QueueModelHistoryList*) m_queue_models[i])->getFracRequestsUsingAnalyticalModel();
            num_queue_models ++;
         }
      }

      queue_utilization /= num_queue_models;
      frac_requests_using_analytical_model /= num_queue_models;

      out << "    Queue Utilization(\%): " << queue_utilization * 100 << endl;
      out << "    Analytical Model Used(\%): " << frac_requests_using_analytical_model * 100 << endl;
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

pair<bool,SInt32>
NetworkModelEMeshHopByHop::computeCoreCountConstraints(SInt32 core_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(core_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * core_count / mesh_width);

   assert(core_count <= mesh_width * mesh_height);
   assert(core_count > (mesh_width - 1) * mesh_height);
   assert(core_count > mesh_width * (mesh_height - 1));

   return make_pair(true,mesh_height * mesh_width);
}

pair<bool, vector<core_id_t> >
NetworkModelEMeshHopByHop::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(core_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * core_count / mesh_width);
   
   assert(core_count == (mesh_height * mesh_width));

   // core_id_list_along_perimeter : list of cores along the perimeter of the chip in clockwise order starting from (0,0)
   vector<core_id_t> core_id_list_along_perimeter;

   for (SInt32 i = 0; i < mesh_width; i++)
      core_id_list_along_perimeter.push_back(i);
   
   for (SInt32 i = 1; i < (mesh_height-1); i++)
      core_id_list_along_perimeter.push_back((i * mesh_width) + mesh_width-1);

   for (SInt32 i = mesh_width-1; i >= 0; i--)
      core_id_list_along_perimeter.push_back(((mesh_height-1) * mesh_width) + i);

   for (SInt32 i = mesh_height-2; i >= 1; i--)
      core_id_list_along_perimeter.push_back(i * mesh_width);

   assert(core_id_list_along_perimeter.size() == (UInt32) (2 * (mesh_width + mesh_height - 2)));

   LOG_ASSERT_ERROR(core_id_list_along_perimeter.size() >= (UInt32) num_memory_controllers,
         "num cores along perimeter(%u), num memory controllers(%i)",
         core_id_list_along_perimeter.size(), num_memory_controllers);

   SInt32 spacing_between_memory_controllers = core_id_list_along_perimeter.size() / num_memory_controllers;
   
   // core_id_list_with_memory_controllers : list of cores that have memory controllers attached to them
   vector<core_id_t> core_id_list_with_memory_controllers;

   for (SInt32 i = 0; i < num_memory_controllers; i++)
   {
      SInt32 index = (i * spacing_between_memory_controllers + mesh_width/2) % core_id_list_along_perimeter.size();
      core_id_list_with_memory_controllers.push_back(core_id_list_along_perimeter[index]);
   }

   return (make_pair(true, core_id_list_with_memory_controllers));
}
