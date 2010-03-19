#include <math.h>
using namespace std;

#include "network_model_emesh_hop_by_hop_generic.h"
#include "core.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "packet_type.h"
#include "queue_model_history_list.h"
#include "memory_manager_base.h"

NetworkModelEMeshHopByHopGeneric::NetworkModelEMeshHopByHopGeneric(Network* net):
   NetworkModel(net),
   m_enabled(false),
   m_num_bytes(0),
   m_num_packets(0),
   m_total_contention_delay(0),
   m_total_latency(0)
{
   SInt32 total_cores = Config::getSingleton()->getTotalCores();
   m_core_id = getNetwork()->getCore()->getId();

   m_mesh_width = (SInt32) floor (sqrt(total_cores));
   m_mesh_height = (SInt32) ceil (1.0 * total_cores / m_mesh_width);

   assert(total_cores == (m_mesh_width * m_mesh_height));
}

NetworkModelEMeshHopByHopGeneric::~NetworkModelEMeshHopByHopGeneric()
{
   for (UInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
   {
      if (m_queue_models[i])
         delete m_queue_models[i];
   }
}

void
NetworkModelEMeshHopByHopGeneric::createQueueModels()
{
   UInt64 min_processing_time = 1;
   // Initialize the queue models for all the '4' output directions
   for (UInt32 direction = 0; direction < NUM_OUTPUT_DIRECTIONS; direction ++)
   {
      m_queue_models[direction] = NULL;
   }

   if ((m_core_id / m_mesh_width) != 0)
   {
      m_queue_models[DOWN] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_core_id % m_mesh_width) != 0)
   {
      m_queue_models[LEFT] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_core_id / m_mesh_width) != (m_mesh_height - 1))
   {
      m_queue_models[UP] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_core_id % m_mesh_width) != (m_mesh_width - 1))
   {
      m_queue_models[RIGHT] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
}

void
NetworkModelEMeshHopByHopGeneric::routePacket(const NetPacket &pkt, vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;
   
   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   LOG_PRINT("pkt length(%u)", pkt_length);

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
            addHop(UP, NetPacket::BROADCAST, computeCoreId(cx,cy+1), pkt.time, pkt_length, nextHops, requester);
         if (cy <= sy)
            addHop(DOWN, NetPacket::BROADCAST, computeCoreId(cx,cy-1), pkt.time, pkt_length, nextHops, requester);
         if (cy == sy)
         {
            if (cx >= sx)
               addHop(RIGHT, NetPacket::BROADCAST, computeCoreId(cx+1,cy), pkt.time, pkt_length, nextHops, requester);
            if (cx <= sx)
               addHop(LEFT, NetPacket::BROADCAST, computeCoreId(cx-1,cy), pkt.time, pkt_length, nextHops, requester);
            if (cx == sx)
               addHop(SELF, m_core_id, m_core_id, pkt.time, pkt_length, nextHops, requester); 
         }
      }
      else
      {
         // Broadcast tree is not enabled
         // Here, broadcast messages are sent as a collection of unicast messages
         LOG_ASSERT_ERROR(pkt.sender == m_core_id,
               "BROADCAST message to be sent at (%i), original sender(%i), Tree not enabled",
               m_core_id, pkt.sender);

         UInt64 curr_time = pkt.time;
         for (core_id_t i = 0; i < (core_id_t) Config::getSingleton()->getTotalCores(); i++)
         {
            // Unicast message to each core
            OutputDirection direction;
            core_id_t next_dest = getNextDest(i, direction);

            addHop(direction, i, next_dest, curr_time, pkt_length, nextHops, requester);

            // Increment the time
            curr_time += computeSerializationLatency(pkt_length);
         }
      }
   }
   else
   {
      // A Unicast packet
      OutputDirection direction;
      core_id_t next_dest = getNextDest(pkt.receiver, direction);

      addHop(direction, pkt.receiver, next_dest, pkt.time, pkt_length, nextHops, requester);
   }
}

void
NetworkModelEMeshHopByHopGeneric::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);
   
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   core_id_t requester = INVALID_CORE_ID;

   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
      requester = getNetwork()->getCore()->getMemoryManager()->getShmemRequester(pkt.data);
   else // Other Packet types
      requester = pkt.sender;
   
   LOG_ASSERT_ERROR((requester >= 0) && (requester < (core_id_t) Config::getSingleton()->getTotalCores()),
         "requester(%i)", requester);

   if ( (!m_enabled) || (requester >= (core_id_t) Config::getSingleton()->getApplicationCores()) )
      return;

   // LOG_ASSERT_ERROR(pkt.start_time > 0, "start_time(%llu)", pkt.start_time);

   UInt64 latency = pkt.time - pkt.start_time;
   UInt64 contention_delay = latency - (computeDistance(pkt.sender, m_core_id) * m_hop_latency);

   if (getNetwork()->getCore()->getId() != pkt.sender)
   {
      UInt64 serialization_latency = computeSerializationLatency(pkt_length);

      latency += serialization_latency;
      pkt.time += serialization_latency;
   }

   m_num_packets ++;
   m_num_bytes += pkt_length;
   m_total_latency += latency;
   m_total_contention_delay += contention_delay;
}

void
NetworkModelEMeshHopByHopGeneric::addHop(OutputDirection direction, 
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
         h.time = pkt_time + computeLatency(direction, pkt_time, pkt_length, requester);

      nextHops.push_back(h);
   }
}

SInt32
NetworkModelEMeshHopByHopGeneric::computeDistance(core_id_t sender, core_id_t receiver)
{
   SInt32 sx, sy, dx, dy;

   computePosition(sender, sx, sy);
   computePosition(receiver, dx, dy);

   return abs(sx - dx) + abs(sy - dy);
}

void
NetworkModelEMeshHopByHopGeneric::computePosition(core_id_t core_id, SInt32 &x, SInt32 &y)
{
   x = core_id % m_mesh_width;
   y = core_id / m_mesh_width;
}

core_id_t
NetworkModelEMeshHopByHopGeneric::computeCoreId(SInt32 x, SInt32 y)
{
   return (y * m_mesh_width + x);
}

UInt64
NetworkModelEMeshHopByHopGeneric::computeLatency(OutputDirection direction, UInt64 pkt_time, UInt32 pkt_length, core_id_t requester)
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

   UInt64 packet_latency = m_hop_latency + queue_delay;

   return packet_latency;
}

UInt64
NetworkModelEMeshHopByHopGeneric::computeSerializationLatency(UInt32 pkt_length)
{
   return computeProcessingTime(pkt_length);
}

UInt64 
NetworkModelEMeshHopByHopGeneric::computeProcessingTime(UInt32 pkt_length)
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
NetworkModelEMeshHopByHopGeneric::getNextDest(SInt32 final_dest, OutputDirection& direction)
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
NetworkModelEMeshHopByHopGeneric::outputSummary(ostream &out)
{
   out << "    bytes received: " << m_num_bytes << endl;
   out << "    packets received: " << m_num_packets << endl;
   if (m_num_packets > 0)
   {
      out << "    average contention delay: " << 
         ((float) m_total_contention_delay / m_num_packets) << endl;
      out << "    average packet latency: " <<
         ((float) m_total_latency / m_num_packets) << endl;
   }
   else
   {
      out << "    average contention delay: " << 
         "NA" << endl;
      out << "    average packet latency: " <<
         "NA" << endl;
   }

   if (m_queue_model_enabled && (m_queue_model_type == "history_list"))
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
NetworkModelEMeshHopByHopGeneric::enable()
{
   m_enabled = true;
}

void
NetworkModelEMeshHopByHopGeneric::disable()
{
   m_enabled = false;
}

bool
NetworkModelEMeshHopByHopGeneric::isEnabled()
{
   return m_enabled;
}

pair<bool,SInt32>
NetworkModelEMeshHopByHopGeneric::computeCoreCountConstraints(SInt32 core_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(core_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * core_count / mesh_width);

   assert(core_count <= mesh_width * mesh_height);
   assert(core_count > (mesh_width - 1) * mesh_height);
   assert(core_count > mesh_width * (mesh_height - 1));

   return make_pair(true,mesh_height * mesh_width);
}

pair<bool, vector<core_id_t> >
NetworkModelEMeshHopByHopGeneric::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 core_count)
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

SInt32
NetworkModelEMeshHopByHopGeneric::computeNumHops(core_id_t sender, core_id_t receiver)
{
   SInt32 core_count = Config::getSingleton()->getTotalCores();

   SInt32 mesh_width = (SInt32) floor (sqrt(core_count));
   // SInt32 mesh_height = (SInt32) ceil (1.0 * core_count / mesh_width);
   
   SInt32 sx, sy, dx, dy;

   sx = sender % mesh_width;
   sy = sender / mesh_width;
   dx = receiver % mesh_width;
   dy = receiver / mesh_width;

   return abs(sx - dx) + abs(sy - dy);
}
