#include <math.h>
using namespace std;

#include "network_model_emesh_hop_by_hop.h"
#include "tile.h"
#include "simulator.h"
#include "config.h"
#include "utils.h"
#include "packet_type.h"

bool NetworkModelEMeshHopByHop::_initialized = false;
SInt32 NetworkModelEMeshHopByHop::_mesh_width;
SInt32 NetworkModelEMeshHopByHop::_mesh_height;
bool NetworkModelEMeshHopByHop::_contention_model_enabled;

NetworkModelEMeshHopByHop::NetworkModelEMeshHopByHop(Network* net, SInt32 network_id)
   : NetworkModel(net, network_id)
{
   try
   {
      // Flit Width is specified in bits
      _flit_width = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/flit_width");

      // Is broadcast tree enabled?
      _has_broadcast_capability = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/broadcast_tree_enabled");
   }
   catch(...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_by_hop parameters from the configuration file");
   }

   // Initialize Topology Params
   initializeEMeshTopologyParams();

   // Create Router & Link Models
   _num_mesh_router_ports = 5;
   createRouterAndLinkModels();
}

NetworkModelEMeshHopByHop::~NetworkModelEMeshHopByHop()
{
   // Destroy the Router & Link Models
   destroyRouterAndLinkModels();
}

void
NetworkModelEMeshHopByHop::initializeEMeshTopologyParams()
{
   if (_initialized)
      return;
   _initialized = true;

   SInt32 num_application_tiles = Config::getSingleton()->getApplicationTiles();

   _mesh_width = (SInt32) floor (sqrt(num_application_tiles));
   _mesh_height = (SInt32) ceil (1.0 * num_application_tiles / _mesh_width);
   LOG_ASSERT_ERROR(num_application_tiles == (_mesh_width * _mesh_height),
         "Num Application Tiles(%i), Mesh Width(%i), Mesh Height(%i)",
         num_application_tiles, _mesh_width, _mesh_height);
      
   try
   {
      // Is contention model enabled?
      _contention_model_enabled = Sim()->getCfg()->getBool("network/emesh_hop_by_hop/queue_model/enabled");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read parameters from the emesh_hop_by_hop section of the cfg file");
   }
}

void
NetworkModelEMeshHopByHop::createRouterAndLinkModels()
{
   if (isSystemTile(_tile_id))
      return;

   // Create Router & Link Models
   // Router & Link are clocked at the same frequency
 
   // Router
   UInt64 router_delay = 0;
   UInt32 num_flits_per_output_buffer = 0;
   // Link
   string link_type;
   UInt64 link_delay = 0;
   // Contention Model
   string contention_model_type;
   try
   {
      // Router Delay (pipeline delay) is specified in cycles
      router_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_by_hop/router/delay");
      // Number of flits per port - used only for power modeling purposes now
      num_flits_per_output_buffer = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/router/num_flits_per_port_buffer");
     
      // Link Parameters
      link_delay = Sim()->getCfg()->getInt("network/emesh_hop_by_hop/link/delay");
      link_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/link/type");

      // Queue Model enabled? If no, this degrades into a hop counter model
      contention_model_type = Sim()->getCfg()->getString("network/emesh_hop_by_hop/queue_model/type");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read emesh router & link parameters from the cfg file");
   }

   // Create the injection port contention model first
   _injection_router = new RouterModel(this, _frequency, _voltage,
                                       1, 1,
                                       4, 0, _flit_width,
                                       _contention_model_enabled, contention_model_type);
   // Mesh Router
   _mesh_router = new RouterModel(this, _frequency, _voltage,
                                  _num_mesh_router_ports, _num_mesh_router_ports,
                                  num_flits_per_output_buffer, router_delay, _flit_width,
                                  _contention_model_enabled, contention_model_type);
   // Mesh Link List
   double link_length = _tile_width;
   _mesh_link_list.resize(_num_mesh_router_ports);
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
   {
      _mesh_link_list[i] = new ElectricalLinkModel(this, link_type,
                                                   _frequency, _voltage,
                                                   link_length, _flit_width);
      assert(_mesh_link_list[i]->getDelay() == link_delay);
   }
}

void
NetworkModelEMeshHopByHop::destroyRouterAndLinkModels()
{
   if (isSystemTile(_tile_id))
      return;

   // Injection Port Router
   delete _injection_router;
   // ENet Router
   delete _mesh_router;
   // ENet Link List
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      delete _mesh_link_list[i];
}

void
NetworkModelEMeshHopByHop::routePacket(const NetPacket &pkt, queue<Hop> &next_hops)
{
   tile_id_t pkt_sender = TILE_ID(pkt.sender);
   tile_id_t pkt_receiver = TILE_ID(pkt.receiver);

   if (pkt.node_type == SEND_TILE)
   {
      UInt64 zero_load_delay = 0;
      UInt64 contention_delay = 0;
      _injection_router->processPacket(pkt, 0, zero_load_delay, contention_delay);
      
      Hop hop(pkt, _tile_id, EMESH, Latency(0,_frequency), Latency(contention_delay,_frequency));
      next_hops.push(hop);
   }

   else if (pkt.node_type == EMESH)
   {
      if (pkt_receiver == NetPacket::BROADCAST)
      {
         SInt32 sx, sy, cx, cy;
         computePosition(pkt_sender, sx, sy);
         computePosition(_tile_id, cx, cy);

         list<NextDest> next_dest_list;

         if (cy >= sy)
            next_dest_list.push_back(NextDest(computeTileID(cx,cy+1), UP, EMESH));
         if (cy <= sy)
            next_dest_list.push_back(NextDest(computeTileID(cx,cy-1), DOWN, EMESH));
         if (cy == sy)
         {
            if (cx >= sx)
               next_dest_list.push_back(NextDest(computeTileID(cx+1,cy), RIGHT, EMESH));
            if (cx <= sx)
               next_dest_list.push_back(NextDest(computeTileID(cx-1,cy), LEFT, EMESH));
         }
         next_dest_list.push_back(NextDest(_tile_id, SELF, RECEIVE_TILE));

         UInt64 zero_load_delay = 0;
         UInt64 contention_delay = 0;
        
         // Get the link delay as well as a vector of directions
         // Remove the tile_ids' that are invalid
         UInt64 max_link_delay = 0;
         vector<SInt32> output_port_list;
         for (list<NextDest>::iterator it = next_dest_list.begin(); it != next_dest_list.end(); )
         {
            if ((*it)._tile_id != INVALID_TILE_ID)
            {
               SInt32 output_port = (*it)._output_port;
               output_port_list.push_back(output_port);
            
               UInt64 link_delay = 0;
               _mesh_link_list[output_port]->processPacket(pkt, link_delay);
               max_link_delay = max<UInt64>(max_link_delay, link_delay);

               it ++;
            }
            else
            {
               it = next_dest_list.erase(it);
            }
         }
         // Update the zero_load_delay
         zero_load_delay += max_link_delay;

         // Get the router to process the packet
         _mesh_router->processPacket(pkt, output_port_list, zero_load_delay, contention_delay);

         // Populate the next_hops queue
         for (list<NextDest>::iterator it = next_dest_list.begin(); it != next_dest_list.end(); it++)
         {
            Hop hop(pkt, (*it)._tile_id, (*it)._node_type, Latency(zero_load_delay,_frequency), Latency(contention_delay,_frequency));
            next_hops.push(hop);
         }
      }

      else // (pkt_receiver != NetPacket::BROADCAST)
      {
         SInt32 cx, cy, dx, dy;
         computePosition(_tile_id, cx, cy);
         computePosition(pkt_receiver, dx, dy);

         NextDest next_dest;

         if (cx > dx)
            next_dest = NextDest(computeTileID(cx-1,cy), LEFT, EMESH);
         else if (cx < dx)
            next_dest = NextDest(computeTileID(cx+1,cy), RIGHT, EMESH);
         else if (cy > dy)
            next_dest = NextDest(computeTileID(cx,cy-1), DOWN, EMESH);
         else if (cy < dy)
            next_dest = NextDest(computeTileID(cx,cy+1), UP, EMESH);
         else
            next_dest = NextDest(_tile_id, SELF, RECEIVE_TILE);

         UInt64 zero_load_delay = 0;
         UInt64 contention_delay = 0;

         assert(next_dest._output_port >= 0 && next_dest._output_port < (SInt32) _mesh_link_list.size());

         // Go through router
         _mesh_router->processPacket(pkt, next_dest._output_port, zero_load_delay, contention_delay);
         // Go through link
         _mesh_link_list[next_dest._output_port]->processPacket(pkt, zero_load_delay);

         assert(next_dest._tile_id != INVALID_TILE_ID);
         Hop hop(pkt, next_dest._tile_id, next_dest._node_type, Latency(zero_load_delay,_frequency), Latency(contention_delay,_frequency));
         next_hops.push(hop);
      
      } // (pkt_receiver == NetPacket::BROADCAST)

   }

   else
   {
      LOG_PRINT_ERROR("Unrecognized Node Type(%i)", pkt.node_type);
   }
}

void
NetworkModelEMeshHopByHop::computePosition(tile_id_t tile_id, SInt32 &x, SInt32 &y)
{
   x = tile_id % _mesh_width;
   y = tile_id / _mesh_width;
}

tile_id_t
NetworkModelEMeshHopByHop::computeTileID(SInt32 x, SInt32 y)
{
   if ( (x < 0) || (y < 0) || (x >= _mesh_width) || (y >= _mesh_height) )
      return INVALID_TILE_ID;
   else
      return (y * _mesh_width + x);
}

SInt32
NetworkModelEMeshHopByHop::computeDistance(tile_id_t sender, tile_id_t receiver)
{
   SInt32 num_application_tiles = (SInt32) Config::getSingleton()->getApplicationTiles();
   SInt32 mesh_width = (SInt32) floor(sqrt(1.0 * num_application_tiles));
   __attribute__((unused)) SInt32 mesh_height = (SInt32) ceil(1.0 * num_application_tiles / mesh_width);

   SInt32 sx, sy, dx, dy;
   sx = sender % mesh_width;
   sy = sender / mesh_width;
   dx = receiver % mesh_width;
   dy = receiver / mesh_width;

   return abs(sx-dx) + abs(sy-dy);
}

void
NetworkModelEMeshHopByHop::outputSummary(ostream &out, const Time& target_completion_time)
{
   NetworkModel::outputSummary(out, target_completion_time);
   outputPowerSummary(out, target_completion_time);
   outputEventCountSummary(out);
   if (_contention_model_enabled)
      outputContentionModelsSummary(out);
}

bool
NetworkModelEMeshHopByHop::isTileCountPermissible(SInt32 tile_count)
{
   SInt32 mesh_width = (SInt32) floor (sqrt(tile_count));
   SInt32 mesh_height = (SInt32) ceil (1.0 * tile_count / mesh_width);

   if (tile_count != (mesh_width * mesh_height))
   {
      fprintf(stderr, "ERROR: Tile Count(%i) != Mesh Width(%i) * Mesh Height(%i)\n", tile_count, mesh_width, mesh_height);
      return false;
   }
   return true;
}

pair<bool, vector<tile_id_t> >
NetworkModelEMeshHopByHop::computeMemoryControllerPositions(SInt32 num_memory_controllers, SInt32 tile_count)
{
   // tile_id_list_along_perimeter : list of tiles along the perimeter of 
   // the chip in clockwise order starting from (0,0)
   
   // Initialize mesh_width, mesh_height
   initializeEMeshTopologyParams();
   
   vector<tile_id_t> tile_id_list_with_memory_controllers;
   // Do a greedy mapping here
   SInt32 memory_controller_mesh_width = (SInt32) floor(sqrt(num_memory_controllers));
   SInt32 memory_controller_mesh_height = (SInt32) ceil(1.0 * num_memory_controllers / memory_controller_mesh_width);

   SInt32 num_computed_memory_controllers = 0;
   for (SInt32 j = 0; j < (memory_controller_mesh_height) && (num_computed_memory_controllers < num_memory_controllers); j++)
   {
      for (SInt32 i = 0; (i < memory_controller_mesh_width) && (num_computed_memory_controllers < num_memory_controllers); i++)
      {
         SInt32 size_x = _mesh_width / memory_controller_mesh_width;
         SInt32 size_y = _mesh_height / memory_controller_mesh_height;
         SInt32 base_x = i * size_x;
         SInt32 base_y = j * size_y;

         if (i == (memory_controller_mesh_width-1))
         {
            size_x = _mesh_width - ((memory_controller_mesh_width-1) * size_x);
         }
         if (j == (memory_controller_mesh_height-1))
         {
            size_y = _mesh_height - ((memory_controller_mesh_height-1) * size_y);
         }

         SInt32 pos_x = base_x + size_x/2;
         SInt32 pos_y = base_y + size_y/2;
         tile_id_t tile_id_with_memory_controller = pos_x + (pos_y * _mesh_width);
         tile_id_list_with_memory_controllers.push_back(tile_id_with_memory_controller);
         num_computed_memory_controllers ++;
      }
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
   SInt32 proc_mesh_width = (SInt32) floor(sqrt(process_count));
   SInt32 proc_mesh_height = (SInt32) floor(1.0 * process_count / proc_mesh_width);

   SInt32 mesh_height_l = (SInt32) ((1.0 *  _mesh_height * proc_mesh_width * proc_mesh_height) / process_count);
   
   for (SInt32 i = 0; i < proc_mesh_width; i++)
   {
      for (SInt32 j = 0; j < proc_mesh_height; j++)
      {
         SInt32 size_x = _mesh_width / proc_mesh_width;
         SInt32 size_y = mesh_height_l / proc_mesh_height;
         SInt32 base_x = i * size_x;
         SInt32 base_y = j * size_y;

         if (i == (proc_mesh_width-1))
         {
            size_x = _mesh_width - ((proc_mesh_width-1) * size_x);
         }
         if (j == (proc_mesh_height-1))
         {
            size_y = mesh_height_l - ((proc_mesh_height-1) * size_y);
         }

         for (SInt32 ii = 0; ii < size_x; ii++)
         {
            for (SInt32 jj = 0; jj < size_y; jj++)
            {
               tile_id_t tile_id = (base_x + ii) + ((base_y + jj) * _mesh_width);
               process_to_tile_mapping[i + j*proc_mesh_width].push_back(tile_id);
            }
         }
      }
   }

   UInt32 procs_left = process_count - (proc_mesh_width * proc_mesh_height);
   for (UInt32 i = proc_mesh_width * proc_mesh_height; i < process_count; i++)
   {
      SInt32 size_x = _mesh_width / procs_left;
      SInt32 size_y = _mesh_height - mesh_height_l;
      SInt32 base_x = (i - (proc_mesh_width * proc_mesh_height)) * size_x;
      SInt32 base_y = mesh_height_l;

      if (i == (process_count-1))
      {
         size_x = _mesh_width - ((procs_left-1) * size_x);
      }

      for (SInt32 ii = 0; ii < size_x; ii++)
      {
         for (SInt32 jj = 0; jj < size_y; jj++)
         {
            tile_id_t tile_id = (base_x + ii) + ((base_y + jj) * _mesh_width);
            process_to_tile_mapping[i].push_back(tile_id);
         }
      }
   }

   return (make_pair(true, process_to_tile_mapping));
}

void
NetworkModelEMeshHopByHop::outputEventCountSummary(ostream& out)
{
   out << "    Event Counters:" << endl;

   if (isApplicationTile(_tile_id))
   {
      out << "      Buffer Writes: " << _mesh_router->getTotalBufferWrites() << endl;
      out << "      Buffer Reads: " << _mesh_router->getTotalBufferReads() << endl;
      out << "      Switch Allocator Requests: " << _mesh_router->getTotalSwitchAllocatorRequests() << endl;
      for (SInt32 i = 1; i <= _num_mesh_router_ports; i++)
         out << "      Crossbar[" << i << "] Traversals: " << _mesh_router->getTotalCrossbarTraversals(i) << endl;
      
      UInt64 total_link_traversals = 0;
      for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
         total_link_traversals += _mesh_link_list[i]->getTotalTraversals();
      out << "      Link Traversals: " << total_link_traversals << endl;
   }

   else if (isSystemTile(_tile_id))
   {
      out << "      Buffer Writes: " << endl;
      out << "      Buffer Reads: " << endl;
      out << "      Switch Allocator Requests: " << endl;
      for (SInt32 i = 1; i <= _num_mesh_router_ports; i++)
         out << "      Crossbar[" << i << "] Traversals: " << endl;
      out << "      Link Traversals: " << endl;
   }

   else
   {
      LOG_PRINT_ERROR("Unrecognized Tile ID(%i)", _tile_id);
   }
}

void
NetworkModelEMeshHopByHop::outputContentionModelsSummary(ostream& out)
{
   out << "    Contention Counters:" << endl;

   if (isApplicationTile(_tile_id))
   {
      out << "      Average EMesh Router Contention Delay: " << _mesh_router->getAverageContentionDelay(0, _num_mesh_router_ports-1) << endl;
      out << "      Average EMesh Router Link Utilization: " << _mesh_router->getAverageLinkUtilization(0, _num_mesh_router_ports-1) << endl;
      out << "      Analytical Models Used (%): " << _mesh_router->getPercentAnalyticalModelsUsed(0, _num_mesh_router_ports-1) << endl;
   }

   else if (isSystemTile(_tile_id))
   {
      out << "      Average EMesh Router Contention Delay: " << endl;
      out << "      Average EMesh Router Link Utilization: " << endl;
      out << "      Analytical Models Used (%): " << endl;
   }

   else
   {
      LOG_PRINT_ERROR("Unrecognized Tile ID(%i)", _tile_id);
   }
}

void
NetworkModelEMeshHopByHop::outputPowerSummary(ostream& out, const Time& target_completion_time)
{
   if (!Config::getSingleton()->getEnablePowerModeling())
      return;

   // Output to sim.out
   out << "    Power Model Statistics: " << endl;
   if (isApplicationTile(_tile_id))
   {
      // Convert time into seconds
      double target_completion_sec = target_completion_time.toSec();
      
      // Compute the final leakage/dynamic energy
      computeEnergy(target_completion_time);
      
      double static_energy = _mesh_router->getPowerModel()->getStaticEnergy();
      double dynamic_energy = _mesh_router->getPowerModel()->getDynamicEnergy();
      for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      {
         static_energy += _mesh_link_list[i]->getPowerModel()->getStaticEnergy();
         dynamic_energy += _mesh_link_list[i]->getPowerModel()->getDynamicEnergy();
      }
      out << "      Average Static Power (in W): " << static_energy / target_completion_sec << endl;
      out << "      Average Dynamic Power (in W): " << dynamic_energy / target_completion_sec << endl;
      out << "      Total Static Energy (in J): " << static_energy << endl;
      out << "      Total Dynamic Energy (in J): " << dynamic_energy << endl;
   }
   else if (isSystemTile(_tile_id))
   {
      out << "      Average Static Power (in W): " << endl;
      out << "      Average Dynamic Power (in W): " << endl;
      out << "      Total Static Energy (in J): " << endl;
      out << "      Total Dynamic Energy (in J): " << endl;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Tile ID(%i)", _tile_id);
   }
}

void
NetworkModelEMeshHopByHop::setDVFS(double frequency, double voltage, const Time& curr_time)
{
   if (!Config::getSingleton()->getEnablePowerModeling())
      return;

   _mesh_router->getPowerModel()->setDVFS(frequency, voltage, curr_time);
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      _mesh_link_list[i]->getPowerModel()->setDVFS(frequency, voltage, curr_time);
}

void
NetworkModelEMeshHopByHop::computeEnergy(const Time& curr_time)
{
   assert(Config::getSingleton()->getEnablePowerModeling());
   _mesh_router->getPowerModel()->computeEnergy(curr_time);
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      _mesh_link_list[i]->getPowerModel()->computeEnergy(curr_time);
}

double
NetworkModelEMeshHopByHop::getDynamicEnergy()
{
   assert(Config::getSingleton()->getEnablePowerModeling());
   double dynamic_energy = _mesh_router->getPowerModel()->getDynamicEnergy();
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      dynamic_energy += _mesh_link_list[i]->getPowerModel()->getDynamicEnergy();
   return dynamic_energy;
}

double
NetworkModelEMeshHopByHop::getStaticEnergy()
{
   assert(Config::getSingleton()->getEnablePowerModeling());
   double static_energy = _mesh_router->getPowerModel()->getStaticEnergy();
   for (SInt32 i = 0; i < _num_mesh_router_ports; i++)
      static_energy += _mesh_link_list[i]->getPowerModel()->getStaticEnergy();
   return static_energy;
}
