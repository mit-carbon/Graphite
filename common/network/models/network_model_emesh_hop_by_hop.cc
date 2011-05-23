#include <math.h>
using namespace std;

#include "network_model_emesh_hop_by_hop.h"
#include "tile.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "packet_type.h"
#include "queue_model_history_list.h"
#include "queue_model_history_tree.h"

SInt32 NetworkModelEMeshHopByHop::m_mesh_width = 0;
SInt32 NetworkModelEMeshHopByHop::m_mesh_height = 0;

NetworkModelEMeshHopByHop::NetworkModelEMeshHopByHop(Network* net, SInt32 network_id):
   NetworkModel(net, network_id)
{
   m_tile_id = getNetwork()->getTile()->getId();
   
   try
   {
      // Network Frequency is specified in GHz
      m_frequency = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop/frequency");
      // Link Width is specified in bits
      m_link_width = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/link/width");
      // Link Length in mm
      m_link_length = Sim()->getCfg()->getFloat("network/emesh_hop_by_hop/link/length");
      // Link Type
      m_link_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/link/type");
      // Router Delay (pipeline delay) is specified in cycles
      m_router_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_by_hop/router/delay");
      // Number of flits per port - Used for power modeling purposes now - Should be used later for performance modeling
      m_num_flits_per_output_buffer = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/router/num_flits_per_port_buffer");

      // Is broadcast tree enabled?
      m_broadcast_tree_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/broadcast_tree_enabled");

      // Queue Model enabled? If no, this degrades into a hop counter model
      m_queue_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/queue_model/enabled");
      m_queue_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/queue_model/type");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_by_hop parameters from the configuration file");
   }

   initializeModels();
}

NetworkModelEMeshHopByHop::~NetworkModelEMeshHopByHop()
{
   // Destroy the Router & Link Models
   destroyRouterAndLinkModels();

   // Destroy the Queue Models
   destroyQueueModels();
}

void
NetworkModelEMeshHopByHop::initializeModels()
{
   // Create Queue Models
   createQueueModels();

   // Create Router & Link Models
   createRouterAndLinkModels();
}

void
NetworkModelEMeshHopByHop::initializeEMeshTopologyParams()
{
   SInt32 total_tiles = Config::getSingleton()->getTotalTiles();

   m_mesh_width = (SInt32) floor (sqrt(total_tiles));
   m_mesh_height = (SInt32) ceil (1.0 * total_tiles / m_mesh_width);
   LOG_ASSERT_ERROR(total_tiles == (m_mesh_width * m_mesh_height),
         "total_tiles(%i), m_mesh_width(%i), m_mesh_height(%i)",
         total_tiles, m_mesh_width, m_mesh_height);
}

void
NetworkModelEMeshHopByHop::createQueueModels()
{
   UInt64 min_processing_time = 1;
   // Initialize the queue models for all the '4' output directions
   for (UInt32 direction = 0; direction < NUM_OUTPUT_DIRECTIONS; direction ++)
   {
      m_queue_models[direction] = NULL;
   }

   if ((m_tile_id / m_mesh_width) != 0)
   {
      m_queue_models[DOWN] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_tile_id % m_mesh_width) != 0)
   {
      m_queue_models[LEFT] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_tile_id / m_mesh_width) != (m_mesh_height - 1))
   {
      m_queue_models[UP] = QueueModel::create(m_queue_model_type, min_processing_time);
   }
   if ((m_tile_id % m_mesh_width) != (m_mesh_width - 1))
   {
      m_queue_models[RIGHT] = QueueModel::create(m_queue_model_type, min_processing_time);
   }

   m_injection_port_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
   m_ejection_port_queue_model = QueueModel::create(m_queue_model_type, min_processing_time);
}

void
NetworkModelEMeshHopByHop::destroyQueueModels()
{
   for (UInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
   {
      if (m_queue_models[i])
         delete m_queue_models[i];
   }

   delete m_injection_port_queue_model;
   delete m_ejection_port_queue_model;
}

void
NetworkModelEMeshHopByHop::resetQueueModels()
{
   destroyQueueModels();
   createQueueModels();
}

void
NetworkModelEMeshHopByHop::createRouterAndLinkModels()
{
   // Create Router & Link Models
   // Right now,
   // Router model yields only power
   // Link model yields delay & power
   // They both will be later augmented to yield area

   // Assume,
   // Router & Link have the same throughput (flit_width = phit_width)
   // Router & Link are clocked at the same frequency
   m_num_router_ports = 5;

   m_electrical_router_model = ElectricalNetworkRouterModel::create(m_num_router_ports,
         m_num_router_ports, m_num_flits_per_output_buffer, m_link_width);
   m_electrical_link_model = ElectricalNetworkLinkModel::create(m_link_type,
         m_frequency,
         m_link_length,
         m_link_width);

   // It is possible that one hop can be accomodated in one cycles by
   // intelligent circuit design but for simplicity, here we consider
   // that a link takes 1 cycle

   // NetworkLinkModel::getDelay() gets delay in cycles (clock frequency is the link frequency)
   // The link frequency is the same as the network frequency here
   UInt64 link_delay = m_electrical_link_model->getDelay();
   LOG_ASSERT_WARNING(link_delay <= 1, "Network Link Delay(%llu) exceeds 1 cycle", link_delay);
   
   m_hop_latency = m_router_delay + link_delay;

   initializeActivityCounters();
}

void
NetworkModelEMeshHopByHop::initializeActivityCounters()
{
   // Initialize Activity Counters
   m_switch_allocator_traversals = 0;
   m_crossbar_traversals = 0;
   m_buffer_accesses = 0;
   m_link_traversals = 0;
}

void
NetworkModelEMeshHopByHop::destroyRouterAndLinkModels()
{
   delete m_electrical_router_model;
   delete m_electrical_link_model;
}

UInt32
NetworkModelEMeshHopByHop::computeAction(const NetPacket& pkt)
{
   if (TILE_ID(pkt.receiver) == NetPacket::BROADCAST)
   {
      LOG_ASSERT_ERROR(m_broadcast_tree_enabled, "pkt.sender.tile_id(%i), pkt.receiver.tile_id(%i)",
            TILE_ID(pkt.sender), TILE_ID(pkt.receiver));
      if (TILE_ID(pkt.sender) == m_tile_id)
      {
         // Dont call routePacket() recursively
         return RoutingAction::RECEIVE;
      }
      else
      {
         // Need to receive as well as forward the packet
         return (RoutingAction::FORWARD | RoutingAction::RECEIVE);
      }
   }
   else if (TILE_ID(pkt.receiver) == m_tile_id)
   {
      return RoutingAction::RECEIVE;
   }
   else
   {
      return RoutingAction::FORWARD;
   }
}

void
NetworkModelEMeshHopByHop::routePacket(const NetPacket &pkt, vector<Hop> &nextHops)
{
   ScopedLock sl(m_lock);

   tile_id_t requester = getRequester(pkt);

   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);

   LOG_PRINT("pkt length(%u)", pkt_length);

   if (TILE_ID(pkt.receiver) == NetPacket::BROADCAST)
   {
      if (m_broadcast_tree_enabled)
      {
         // Injection Port Modeling
         UInt64 injection_port_queue_delay = 0;
         if (TILE_ID(pkt.sender) == m_tile_id)
            injection_port_queue_delay = computeInjectionPortQueueDelay(pkt, TILE_ID(pkt.receiver), pkt.time, pkt_length);

         UInt64 curr_time = pkt.time + injection_port_queue_delay;         

         // Broadcast tree is enabled
         // Build the broadcast tree
         SInt32 sx, sy, cx, cy;
            
         computePosition(TILE_ID(pkt.sender), sx, sy);
         computePosition(m_tile_id, cx, cy);

         if (cy >= sy)
            addHop(UP, NetPacket::BROADCAST, computeTileId(cx,cy+1), pkt, curr_time, pkt_length, nextHops, requester);
         if (cy <= sy)
            addHop(DOWN, NetPacket::BROADCAST, computeTileId(cx,cy-1), pkt, curr_time, pkt_length, nextHops, requester);
         if (cy == sy)
         {
            if (cx >= sx)
               addHop(RIGHT, NetPacket::BROADCAST, computeTileId(cx+1,cy), pkt, curr_time, pkt_length, nextHops, requester);
            if (cx <= sx)
               addHop(LEFT, NetPacket::BROADCAST, computeTileId(cx-1,cy), pkt, curr_time, pkt_length, nextHops, requester);
            if (cx == sx)
               addHop(SELF, NetPacket::BROADCAST, m_tile_id, pkt, curr_time, pkt_length, nextHops, requester); 
         }
      }
      else
      {
         // Broadcast tree is not enabled
         // Here, broadcast messages are sent as a collection of unicast messages
         LOG_ASSERT_ERROR(TILE_ID(pkt.sender) == m_tile_id,
               "BROADCAST message to be sent at (%i), original sender(%i), Tree not enabled",
               m_tile_id, TILE_ID(pkt.sender));

         for (tile_id_t i = 0; i < (tile_id_t) Config::getSingleton()->getTotalTiles(); i++)
         {
            // Injection Port Modeling
            UInt64 injection_port_queue_delay = computeInjectionPortQueueDelay(pkt, i, pkt.time, pkt_length);
            UInt64 curr_time = pkt.time + injection_port_queue_delay;         

            // Unicast message to each tile
            OutputDirection direction;
            tile_id_t next_dest = getNextDest(i, direction);

            addHop(direction, i, next_dest, pkt, curr_time, pkt_length, nextHops, requester);
         }
      }
   }
   else
   {
      // Injection Port Modeling
      UInt64 injection_port_queue_delay = 0;
      if (TILE_ID(pkt.sender) == m_tile_id)
         injection_port_queue_delay = computeInjectionPortQueueDelay(pkt, TILE_ID(pkt.receiver), pkt.time, pkt_length);
      UInt64 curr_time = pkt.time + injection_port_queue_delay;         
      
      // A Unicast packet
      OutputDirection direction;
      tile_id_t next_dest = getNextDest(TILE_ID(pkt.receiver), direction);

      addHop(direction, TILE_ID(pkt.receiver), next_dest, pkt, curr_time, pkt_length, nextHops, requester);
   }
}

void
NetworkModelEMeshHopByHop::processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(m_lock);
   
   tile_id_t requester = getRequester(pkt);
   if ( (!isEnabled()) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) )
      return;

   UInt64 zero_load_latency = computeDistance(TILE_ID(pkt.sender), m_tile_id) * m_hop_latency;

   if (TILE_ID(pkt.sender) != m_tile_id)
   {
      UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
      UInt64 processing_time = computeProcessingTime(pkt_length);
      UInt64 ejection_port_queue_delay = computeEjectionPortQueueDelay(pkt, pkt.time, pkt_length);

      zero_load_latency += processing_time;
      pkt.time += (ejection_port_queue_delay + processing_time);
   }

   updateReceiveCounters(pkt, zero_load_latency);
}

void
NetworkModelEMeshHopByHop::addHop(OutputDirection direction, 
      tile_id_t final_dest, tile_id_t next_dest,
      const NetPacket& pkt,
      UInt64 pkt_time, UInt32 pkt_length,
      vector<Hop>& nextHops, tile_id_t requester)
{
   LOG_ASSERT_ERROR((direction == SELF) || ((direction >= 0) && (direction < NUM_OUTPUT_DIRECTIONS)),
         "Invalid Direction(%u)", direction);

   if ((direction == SELF) || m_queue_models[direction])
   {
      Hop h;
      TILE_ID(h.final_dest) = final_dest;
      TILE_ID(h.next_dest) = next_dest;

      if (direction == SELF)
         h.time = pkt_time;
      else
         h.time = pkt_time + computeLatency(direction, pkt, pkt_time, pkt_length, requester);

      nextHops.push_back(h);
   }
}

SInt32
NetworkModelEMeshHopByHop::computeDistance(tile_id_t sender, tile_id_t receiver)
{
   SInt32 sx, sy, dx, dy;

   computePosition(sender, sx, sy);
   computePosition(receiver, dx, dy);

   return abs(sx - dx) + abs(sy - dy);
}

void
NetworkModelEMeshHopByHop::computePosition(tile_id_t tile_id, SInt32 &x, SInt32 &y)
{
   x = tile_id % m_mesh_width;
   y = tile_id / m_mesh_width;
}

tile_id_t
NetworkModelEMeshHopByHop::computeTileId(SInt32 x, SInt32 y)
{
   return (y * m_mesh_width + x);
}

UInt64
NetworkModelEMeshHopByHop::computeLatency(OutputDirection direction, const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length, tile_id_t requester)
{
   LOG_ASSERT_ERROR((direction >= 0) && (direction < NUM_OUTPUT_DIRECTIONS),
         "Invalid Direction(%u)", direction);

   if ( (!isEnabled()) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) )
      return 0;

   UInt64 processing_time = computeProcessingTime(pkt_length);

   // Calculate the contention delay
   UInt64 queue_delay = 0;
   if (m_queue_model_enabled)
      queue_delay = m_queue_models[direction]->computeQueueDelay(pkt_time, processing_time, requester);

   // Update Dynamic Energy State of the Router & Link
   updateDynamicEnergy(pkt       /* Packet */,
         queue_delay             /* is_buffered */,
         m_num_router_ports/2    /* contention */);

   LOG_PRINT("Queue Delay(%llu), Hop Latency(%llu)", queue_delay, m_hop_latency);
   UInt64 packet_latency = m_hop_latency + queue_delay;

   return packet_latency;
}

UInt64
NetworkModelEMeshHopByHop::computeInjectionPortQueueDelay(const NetPacket& pkt, tile_id_t receiver, UInt64 pkt_time, UInt32 pkt_length)
{
   tile_id_t requester = getRequester(pkt);

   if ( (!m_queue_model_enabled) ||
        (!isEnabled()) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (receiver == m_tile_id) )
      return 0;

   UInt64 processing_time = computeProcessingTime(pkt_length);
   return m_injection_port_queue_model->computeQueueDelay(pkt_time, processing_time, requester);
}

UInt64
NetworkModelEMeshHopByHop::computeEjectionPortQueueDelay(const NetPacket& pkt, UInt64 pkt_time, UInt32 pkt_length)
{
   tile_id_t requester = getRequester(pkt);

   if ( (!m_queue_model_enabled) ||
        (!isEnabled()) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) )
      return 0;

   UInt64 processing_time = computeProcessingTime(pkt_length);
   UInt64 ejection_port_queue_delay =  m_ejection_port_queue_model->computeQueueDelay(pkt_time, processing_time, requester);

   // Update Dynamic Energy State of the Router & Link
   updateDynamicEnergy(pkt          /* Packet */,
         ejection_port_queue_delay  /* is_buffered */,
         m_num_router_ports/2       /* contention */);

   return ejection_port_queue_delay;
}

UInt64 
NetworkModelEMeshHopByHop::computeProcessingTime(UInt32 pkt_length)
{
   // Send: (pkt_length * 8) bits
   // Link Width: (m_link_width) bits
   UInt32 num_bits = pkt_length * 8;
   if (num_bits % m_link_width == 0)
      return (UInt64) (num_bits/m_link_width);
   else
      return (UInt64) (num_bits/m_link_width + 1);
}

SInt32
NetworkModelEMeshHopByHop::getNextDest(SInt32 final_dest, OutputDirection& direction)
{
   // Do dimension-order routing
   // Curently, do store-and-forward routing
   // FIXME: Should change this to wormhole routing eventually
   
   SInt32 sx, sy, dx, dy;

   computePosition(m_tile_id, sx, sy);
   computePosition(final_dest, dx, dy);

   if (sx > dx)
   {
      direction = LEFT;
      return computeTileId(sx-1,sy);
   }
   else if (sx < dx)
   {
      direction = RIGHT;
      return computeTileId(sx+1,sy);
   }
   else if (sy > dy)
   {
      direction = DOWN;
      return computeTileId(sx,sy-1);
   }
   else if (sy < dy)
   {
      direction = UP;
      return computeTileId(sx,sy+1);
   }
   else
   {
      // A send to itself
      direction = SELF;
      return m_tile_id;
   }
}

void
NetworkModelEMeshHopByHop::outputSummary(ostream &out)
{
   NetworkModel::outputSummary(out);
   
   if (m_queue_model_enabled && ((m_queue_model_type == "history_list") || (m_queue_model_type == "history_tree")))
   {
      out << "  Queue Models:" << endl;
         
      float queue_utilization = 0.0;
      UInt64 total_requests_using_analytical_model = 0;
      UInt64 total_requests = 0;
      UInt32 num_queue_models = 0;
     
      if (m_queue_model_type == "history_list")
      { 
         for (SInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
         {
            if (m_queue_models[i])
            {
               queue_utilization += ((QueueModelHistoryList*) m_queue_models[i])->getQueueUtilization();
               total_requests_using_analytical_model += ((QueueModelHistoryList*) m_queue_models[i])->getTotalRequestsUsingAnalyticalModel();
               total_requests += ((QueueModelHistoryList*) m_queue_models[i])->getTotalRequests(); 
               num_queue_models ++;
            }
         }

         queue_utilization += ((QueueModelHistoryList*) m_injection_port_queue_model)->getQueueUtilization();
         total_requests_using_analytical_model += ((QueueModelHistoryList*) m_injection_port_queue_model)->getTotalRequestsUsingAnalyticalModel();
         total_requests += ((QueueModelHistoryList*) m_injection_port_queue_model)->getTotalRequests();
         
         queue_utilization += ((QueueModelHistoryList*) m_ejection_port_queue_model)->getQueueUtilization();
         total_requests_using_analytical_model += ((QueueModelHistoryList*) m_ejection_port_queue_model)->getTotalRequestsUsingAnalyticalModel();
         total_requests += ((QueueModelHistoryList*) m_ejection_port_queue_model)->getTotalRequests();
         
         num_queue_models += 2;
      }
      else // m_queue_model_type == "history_tree"
      {
         for (SInt32 i = 0; i < NUM_OUTPUT_DIRECTIONS; i++)
         {
            if (m_queue_models[i])
            {
               queue_utilization += ((QueueModelHistoryTree*) m_queue_models[i])->getQueueUtilization();
               total_requests_using_analytical_model += ((QueueModelHistoryTree*) m_queue_models[i])->getTotalRequestsUsingAnalyticalModel();
               total_requests += ((QueueModelHistoryTree*) m_queue_models[i])->getTotalRequests(); 
               num_queue_models ++;
            }
         }

         queue_utilization += ((QueueModelHistoryTree*) m_injection_port_queue_model)->getQueueUtilization();
         total_requests_using_analytical_model += ((QueueModelHistoryTree*) m_injection_port_queue_model)->getTotalRequestsUsingAnalyticalModel();
         total_requests += ((QueueModelHistoryTree*) m_injection_port_queue_model)->getTotalRequests();
         
         queue_utilization += ((QueueModelHistoryTree*) m_ejection_port_queue_model)->getQueueUtilization();
         total_requests_using_analytical_model += ((QueueModelHistoryTree*) m_ejection_port_queue_model)->getTotalRequestsUsingAnalyticalModel();
         total_requests += ((QueueModelHistoryTree*) m_ejection_port_queue_model)->getTotalRequests();
         
         num_queue_models += 2;
      }

      queue_utilization /= num_queue_models;
      double frac_requests_using_analytical_model = ((double) total_requests_using_analytical_model) / total_requests;

      out << "    Link Utilization(\%): " << queue_utilization * 100 << endl;
      out << "    Analytical Model Used(\%): " << frac_requests_using_analytical_model * 100 << endl;
   }

   outputPowerSummary(out);
}

void
NetworkModelEMeshHopByHop::reset()
{
   // Reset Queue Models
   resetQueueModels();
   
   // Activity Counters
   initializeActivityCounters();
   
   // Reset Router & Link Models
   m_electrical_router_model->resetCounters();
   m_electrical_link_model->resetCounters();
}

pair<bool,SInt32>
NetworkModelEMeshHopByHop::computeTileCountConstraints(SInt32 tile_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(tile_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * tile_count / mesh_width);

   assert(tile_count <= mesh_width * mesh_height);
   assert(tile_count > (mesh_width - 1) * mesh_height);
   assert(tile_count > mesh_width * (mesh_height - 1));

   return make_pair(true,mesh_height * mesh_width);
}

pair<bool, vector<tile_id_t> >
NetworkModelEMeshHopByHop::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count)
{
   // tile_id_list_along_perimeter : list of tiles along the perimeter of 
   // the chip in clockwise order starting from (0,0)
   
   // Initialize mesh_width, mesh_height
   initializeEMeshTopologyParams();

   vector<tile_id_t> tile_id_list_along_perimeter;

   for (SInt32 i = 0; i < m_mesh_width; i++)
      tile_id_list_along_perimeter.push_back(i);
   
   for (SInt32 i = 1; i < (m_mesh_height-1); i++)
      tile_id_list_along_perimeter.push_back((i * m_mesh_width) + m_mesh_width-1);

   for (SInt32 i = m_mesh_width-1; i >= 0; i--)
      tile_id_list_along_perimeter.push_back(((m_mesh_height-1) * m_mesh_width) + i);

   for (SInt32 i = m_mesh_height-2; i >= 1; i--)
      tile_id_list_along_perimeter.push_back(i * m_mesh_width);

   assert(tile_id_list_along_perimeter.size() == (UInt32) (2 * (m_mesh_width + m_mesh_height - 2)));

   LOG_ASSERT_ERROR(tile_id_list_along_perimeter.size() >= (UInt32) num_memory_controllers,
         "num tiles along perimeter(%u), num memory controllers(%i)",
         tile_id_list_along_perimeter.size(), num_memory_controllers);

   SInt32 spacing_between_memory_controllers = tile_id_list_along_perimeter.size() / num_memory_controllers;
   
   // tile_id_list_with_memory_controllers : list of tiles that have memory controllers attached to them
   vector<tile_id_t> tile_id_list_with_memory_controllers;

   for (SInt32 i = 0; i < num_memory_controllers; i++)
   {
      SInt32 index = (i * spacing_between_memory_controllers + m_mesh_width/2) % tile_id_list_along_perimeter.size();
      tile_id_list_with_memory_controllers.push_back(tile_id_list_along_perimeter[index]);
   }

   return (make_pair(true, tile_id_list_with_memory_controllers));
}

pair<bool, vector<Config::TileList> >
NetworkModelEMeshHopByHop::computeProcessToTileMapping()
{
   // Initialize mesh_width, mesh_height
   initializeEMeshTopologyParams();

   UInt32 process_count = Config::getSingleton()->getProcessCount();

   vector<Config::TileList> process_to_tile_mapping(process_count);
   // Do a greedy mapping here
   SInt32 proc_mesh_width = (SInt32) floor(sqrt(process_count));
   SInt32 proc_mesh_height = (SInt32) floor(1.0 * process_count / proc_mesh_width);

   SInt32 mesh_height_l = (SInt32) ((1.0 *  m_mesh_height * proc_mesh_width * proc_mesh_height) / process_count);
   
   for (SInt32 i = 0; i < proc_mesh_width; i++)
   {
      for (SInt32 j = 0; j < proc_mesh_height; j++)
      {
         SInt32 size_x = m_mesh_width / proc_mesh_width;
         SInt32 size_y = mesh_height_l / proc_mesh_height;
         SInt32 base_x = i * size_x;
         SInt32 base_y = j * size_y;

         if (i == (proc_mesh_width-1))
         {
            size_x = m_mesh_width - ((proc_mesh_width-1) * size_x);
         }
         if (j == (proc_mesh_height-1))
         {
            size_y = mesh_height_l - ((proc_mesh_height-1) * size_y);
         }

         for (SInt32 ii = 0; ii < size_x; ii++)
         {
            for (SInt32 jj = 0; jj < size_y; jj++)
            {
               tile_id_t tile_id = (base_x + ii) + ((base_y + jj) * m_mesh_width);
               process_to_tile_mapping[i + j*proc_mesh_width].push_back(tile_id);
            }
         }
      }
   }

   UInt32 procs_left = process_count - (proc_mesh_width * proc_mesh_height);
   for (UInt32 i = proc_mesh_width * proc_mesh_height; i < process_count; i++)
   {
      SInt32 size_x = m_mesh_width / procs_left;
      SInt32 size_y = m_mesh_height - mesh_height_l;
      SInt32 base_x = (i - (proc_mesh_width * proc_mesh_height)) * size_x;
      SInt32 base_y = mesh_height_l;

      if (i == (process_count-1))
      {
         size_x = m_mesh_width - ((procs_left-1) * size_x);
      }

      for (SInt32 ii = 0; ii < size_x; ii++)
      {
         for (SInt32 jj = 0; jj < size_y; jj++)
         {
            tile_id_t tile_id = (base_x + ii) + ((base_y + jj) * m_mesh_width);
            process_to_tile_mapping[i].push_back(tile_id);
         }
      }
   }

   return (make_pair(true, process_to_tile_mapping));
}

SInt32
NetworkModelEMeshHopByHop::computeNumHops(tile_id_t sender, tile_id_t receiver)
{
   SInt32 tile_count = Config::getSingleton()->getTotalTiles();

   SInt32 mesh_width = (SInt32) floor (sqrt(tile_count));
   // SInt32 mesh_height = (SInt32) ceil (1.0 * tile_count / mesh_width);
   
   SInt32 sx, sy, dx, dy;

   sx = sender % mesh_width;
   sy = sender / mesh_width;
   dx = receiver % mesh_width;
   dy = receiver / mesh_width;

   return abs(sx - dx) + abs(sy - dy);
}

void
NetworkModelEMeshHopByHop::updateDynamicEnergy(const NetPacket& pkt,
                                               bool is_buffered,
                                               UInt32 contention)
{
   assert(isEnabled());
   // TODO: Make these models detailed later - Compute exact number of bit flips
   UInt32 pkt_length = getNetwork()->getModeledLength(pkt);
   // For now, assume that half of the bits in the packet flip
   UInt32 num_flits = computeProcessingTime(pkt_length);
      
   // Dynamic Energy Dissipated

   // 1) Electrical Router
   // For every activity, update the dynamic energy due to the clock

   // Assume half of the input ports are contending for the same output port
   // Switch allocation is only done for the head flit. All the other flits just follow.
   // So, we dont need to update dynamic energies again
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      m_electrical_router_model->updateDynamicEnergySwitchAllocator(contention);
      m_electrical_router_model->updateDynamicEnergyClock();
   }
   m_switch_allocator_traversals ++;

   // Assume half of the bits flip while crossing the crossbar 
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      m_electrical_router_model->updateDynamicEnergyCrossbar(m_link_width/2, num_flits); 
      m_electrical_router_model->updateDynamicEnergyClock(num_flits);
   }
   m_crossbar_traversals += num_flits;
  
   // Add the flit_buffer dynamic power. We need to write once and read once
   if (is_buffered)
   {
      if (Config::getSingleton()->getEnablePowerModeling())
      {
         // Buffer Write Energy
         m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::WRITE,
               m_link_width/2, num_flits);
         m_electrical_router_model->updateDynamicEnergyClock(num_flits);
    
         // Buffer Read Energy
         m_electrical_router_model->updateDynamicEnergyBuffer(ElectricalNetworkRouterModel::BufferAccess::READ,
               m_link_width/2, num_flits);
         m_electrical_router_model->updateDynamicEnergyClock(num_flits);
      }

      m_buffer_accesses += num_flits;
   }

   // 2) Electrical Link
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      m_electrical_link_model->updateDynamicEnergy(m_link_width/2, num_flits);
   }
   m_link_traversals += num_flits;
}

void
NetworkModelEMeshHopByHop::outputPowerSummary(ostream& out)
{
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      // We need to get the power of the router + all the outgoing links (a total of 4 outputs)
      volatile double static_power = m_electrical_router_model->getTotalStaticPower() + (m_electrical_link_model->getStaticPower() * NUM_OUTPUT_DIRECTIONS);
      volatile double dynamic_energy = m_electrical_router_model->getTotalDynamicEnergy() + m_electrical_link_model->getDynamicEnergy();

      out << "    Static Power: " << static_power << endl;
      out << "    Dynamic Energy: " << dynamic_energy << endl;
   }

   out << "  Activity Counters:" << endl;
   out << "    Switch Allocator Traversals: " << m_switch_allocator_traversals << endl;
   out << "    Crossbar Traversals: " << m_crossbar_traversals << endl;
   out << "    Buffer Accesses: " << m_buffer_accesses << endl;
   out << "    Link Traversals: " << m_link_traversals << endl;
}
