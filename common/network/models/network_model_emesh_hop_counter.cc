#include <stdlib.h>
#include <math.h>

#include "network_model_emesh_hop_counter.h"
#include "simulator.h"
#include "config.h"
#include "config.h"
#include "tile.h"

NetworkModelEMeshHopCounter::NetworkModelEMeshHopCounter(Network *net, SInt32 network_id)
   : NetworkModel(net, network_id)
   , _router_power_model(NULL)
   , _electrical_link_power_model(NULL)
{
   SInt32 num_application_tiles = Config::getSingleton()->getApplicationTiles();

   _mesh_width = (SInt32) floor (sqrt(num_application_tiles));
   _mesh_height = (SInt32) ceil (1.0 * num_application_tiles / _mesh_width);

   assert(num_application_tiles <= _mesh_width * _mesh_height);
   assert(num_application_tiles > (_mesh_width - 1) * _mesh_height);
   assert(num_application_tiles > _mesh_width * (_mesh_height - 1));
   
   try
   {
      _frequency = Sim()->getCfg()->getFloat("network/emesh_hop_counter/frequency");
      _flit_width = Sim()->getCfg()->getInt("network/emesh_hop_counter/flit_width");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_counter paramters from the cfg file");
   }

   // Broadcast Capability
   _has_broadcast_capability = false;

   createRouterAndLinkModels();
   
   // Initialize event counters
   initializeEventCounters();
}

NetworkModelEMeshHopCounter::~NetworkModelEMeshHopCounter()
{
   // Destroy the Router & Link Models
   destroyRouterAndLinkModels();
}

void
NetworkModelEMeshHopCounter::createRouterAndLinkModels()
{
   if (isSystemTile(_tile_id))
      return;

   // Link parameters
   UInt64 link_delay = 0;
   string link_type;
   double link_length = 0.0;
   // Router parameters
   UInt64 router_delay = 0;   // Delay of the router (in clock cycles)
   UInt32 num_flits_per_output_buffer = 0;   // Here, contention is not modeled
   
   try
   {
      link_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_counter/link/delay");
      link_type = Sim()->getCfg()->getString("network/emesh_hop_counter/link/type");
      link_length = Sim()->getCfg()->getFloat("general/tile_width");

      router_delay = (UInt64) Sim()->getCfg()->getInt("network/emesh_hop_counter/router/delay");
      num_flits_per_output_buffer = Sim()->getCfg()->getInt("network/emesh_hop_counter/router/num_flits_per_port_buffer");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read emesh_hop_counter link and router parameters");
   }

   LOG_ASSERT_ERROR(link_delay == 1, "Network Link Delay(%llu) is not 1 cycle", link_delay);

   // Hop latency
   _hop_latency = router_delay + link_delay;
   
   // Router & Link have the same throughput (flit_width = phit_width = link_width)
   // Router & Link are clocked at the same frequency
   
   // Instantiate router & link power models
   UInt32 num_router_ports = 5;
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _router_power_model = new RouterPowerModel(_frequency, num_router_ports, num_router_ports,
                                                 num_flits_per_output_buffer, _flit_width);
      _electrical_link_power_model = new ElectricalLinkPowerModel(link_type, _frequency, link_length, _flit_width);
   }

}

void
NetworkModelEMeshHopCounter::destroyRouterAndLinkModels()
{
   if (isSystemTile(_tile_id))
      return;
   
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      delete _router_power_model;
      delete _electrical_link_power_model;
   }
}

void
NetworkModelEMeshHopCounter::initializeEventCounters()
{
   _buffer_writes = 0;
   _buffer_reads = 0;
   _switch_allocator_traversals = 0;
   _crossbar_traversals = 0;
   _link_traversals = 0;
}

void
NetworkModelEMeshHopCounter::updateEventCounters(UInt32 num_flits, UInt32 num_hops)
{
   _buffer_writes += (num_flits * num_hops); 
   _buffer_reads += (num_flits * num_hops); 
   _switch_allocator_traversals += num_hops;
   _crossbar_traversals += (num_flits * num_hops);
   _link_traversals += (num_flits * num_hops);
}

void
NetworkModelEMeshHopCounter::computePosition(tile_id_t tile, SInt32 &x, SInt32 &y)
{
   x = tile % _mesh_width;
   y = tile / _mesh_width;
}

SInt32
NetworkModelEMeshHopCounter::computeDistance(SInt32 x1, SInt32 y1, SInt32 x2, SInt32 y2)
{
   return abs(x1 - x2) + abs(y1 - y2);
}

void
NetworkModelEMeshHopCounter::routePacket(const NetPacket &pkt, queue<Hop> &next_hops)
{
   SInt32 sx, sy, dx, dy;

   computePosition(TILE_ID(pkt.sender), sx, sy);
   computePosition(TILE_ID(pkt.receiver), dx, dy);

   UInt32 num_hops = computeDistance(sx, sy, dx, dy);
   UInt64 latency = (isModelEnabled(pkt)) ? (num_hops * _hop_latency) : 0;

   updateDynamicEnergy(pkt, num_hops);

   Hop hop(pkt, TILE_ID(pkt.receiver), RECEIVE_TILE, latency, 0);
   next_hops.push(hop);
}

void
NetworkModelEMeshHopCounter::outputSummary(std::ostream &out)
{
   NetworkModel::outputSummary(out);
   outputPowerSummary(out);
   outputEventCountSummary(out);
}

// Power/Energy related functions
void
NetworkModelEMeshHopCounter::updateDynamicEnergy(const NetPacket& packet, UInt32 num_hops)
{
   if (!isModelEnabled(packet))
      return;

   UInt32 num_flits = computeNumFlits(getModeledLength(packet));
    
   // Update event counters 
   updateEventCounters(num_flits, num_hops);

   // Update energy counters
   if (Config::getSingleton()->getEnablePowerModeling())
   {
      _router_power_model->updateDynamicEnergy(num_flits*num_hops, num_hops);
      _electrical_link_power_model->updateDynamicEnergy(num_flits * num_hops);
   }
}

void
NetworkModelEMeshHopCounter::outputPowerSummary(ostream& out)
{
   if (!Config::getSingleton()->getEnablePowerModeling())
      return;

   out << "    Energy Counters: " << endl;
   if (isApplicationTile(_tile_id))
   {
      // We need to get the power of the router + all the outgoing links (a total of 4 outputs)
      volatile double static_power = _router_power_model->getStaticPower() +
                                     (_electrical_link_power_model->getStaticPower() * _NUM_OUTPUT_DIRECTIONS);
      volatile double dynamic_energy = _router_power_model->getDynamicEnergy() +
                                       _electrical_link_power_model->getDynamicEnergy();
      out << "      Static Power (in W): " << static_power << endl;
      out << "      Dynamic Energy (in J): " << dynamic_energy << endl;
   }
   else if (isSystemTile(_tile_id))
   {
      out << "      Static Power (in W): " << endl;
      out << "      Dynamic Energy (in J): " << endl;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Tile ID(%i)", _tile_id);
   }
}

void
NetworkModelEMeshHopCounter::outputEventCountSummary(ostream& out)
{
   out << "    Event Counters:" << endl;
   if (isApplicationTile(_tile_id))
   {
      out << "      Buffer Writes: " << _buffer_writes << endl;
      out << "      Buffer Reads: " << _buffer_reads << endl;
      out << "      Switch Allocator Traversals: " << _switch_allocator_traversals << endl;
      out << "      Crossbar Traversals: " << _crossbar_traversals << endl;
      out << "      Link Traversals: " << _link_traversals << endl;
   }
   else if (isSystemTile(_tile_id))
   {
      out << "      Buffer Writes: " << endl;
      out << "      Buffer Reads: " << endl;
      out << "      Switch Allocator Traversals: " << endl;
      out << "      Crossbar Traversals: " << endl;
      out << "      Link Traversals: " << endl;
   }
   else
   {
      LOG_PRINT_ERROR("Unrecognized Tile ID(%i)", _tile_id);
   }
}
