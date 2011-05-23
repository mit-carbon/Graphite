#include <cassert>
using namespace std;

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"
#include "network_model_emesh_hop_counter.h"
#include "network_model_analytical.h"
#include "network_model_emesh_hop_by_hop.h"
#include "network_model_eclos.h"
#include "network_model_atac.h"
#include "memory_manager_base.h"
#include "clock_converter.h"
#include "log.h"

NetworkModel::NetworkModel(Network *network, SInt32 network_id):
   _network(network),
   _network_id(network_id),
   _enabled(false)
{
   if (network_id == 0)
      _network_name = "network/user_model_1";
   else if (network_id == 1)
      _network_name = "network/user_model_2";
   else if (network_id == 2)
      _network_name = "network/memory_model_1";
   else if (network_id == 3)
      _network_name = "network/memory_model_2";
   else if (network_id == 4)
      _network_name = "network/system_model";
   else
      LOG_PRINT_ERROR("Unrecognized Network Num(%u)", network_id);

   // Initialize Event Counters
   initializeEventCounters();
}

NetworkModel*
NetworkModel::createModel(Network *net, SInt32 network_id, UInt32 model_type)
{
   switch (model_type)
   {
   case NETWORK_MAGIC:
      return new NetworkModelMagic(net, network_id);

   case NETWORK_EMESH_HOP_COUNTER:
      return new NetworkModelEMeshHopCounter(net, network_id);

   case NETWORK_ANALYTICAL_MESH:
      return new NetworkModelAnalytical(net, network_id);

   case NETWORK_EMESH_HOP_BY_HOP:
      return new NetworkModelEMeshHopByHop(net, network_id);

   case NETWORK_ECLOS:
      return new NetworkModelEClos(net, network_id);

   case NETWORK_ATAC:
      return new NetworkModelAtac(net, network_id);

   default:
      LOG_PRINT_ERROR("Unrecognized Network Model(%u)", model_type);
      return NULL;
   }
}

tile_id_t
NetworkModel::getRequester(const NetPacket& packet)
{
   tile_id_t requester = INVALID_TILE_ID;

   SInt32 network_id = getNetworkId();
   if ((network_id == STATIC_NETWORK_USER_1) || (network_id == STATIC_NETWORK_USER_2))
      requester = TILE_ID(packet.sender);
   else if ((network_id == STATIC_NETWORK_MEMORY_1) || (network_id == STATIC_NETWORK_MEMORY_2))
      requester = getNetwork()->getTile()->getMemoryManager()->getShmemRequester(packet.data);
   else // (network_id == STATIC_NETWORK_SYSTEM)
      requester = INVALID_TILE_ID;
   
   // LOG_ASSERT_ERROR((requester >= 0) && (requester < (tile_id_t) Config::getSingleton()->getTotalTiles()),
   //       "requester(%i)", requester);

   return requester;
}

void
NetworkModel::initializeEventCounters()
{
   _total_packets_sent = 0;
   _total_flits_sent = 0;
   _total_bytes_sent = 0;
   
   _total_packets_broadcasted = 0;
   _total_flits_broadcasted = 0;
   _total_bytes_broadcasted = 0;
   
   _total_packets_received = 0;
   _total_flits_received = 0;
   _total_bytes_received = 0;
   
   _total_packet_latency = 0;
   _total_contention_delay = 0;
}

UInt32
NetworkModel::computeNumFlits(UInt32 packet_length)
{
   UInt32 num_bits = packet_length * 8;
   if ( (num_bits % getFlitWidth()) == 0 )
      return (num_bits / getFlitWidth());
   else
      return ( (num_bits / getFlitWidth()) + 1 );
}

void
NetworkModel::updateSendCounters(const NetPacket& packet)
{
   tile_id_t sender = TILE_ID(packet.sender);
   tile_id_t receiver = TILE_ID(packet.receiver);

   tile_id_t requester = getRequester(packet);
   if ( (!_enabled) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (sender == receiver) )
      return;

   UInt32 packet_length = getNetwork()->getModeledLength(packet);
   UInt32 num_flits = computeNumFlits(packet_length);
   
   _total_packets_sent ++;
   _total_flits_sent += num_flits;
   _total_bytes_sent += packet_length;

   if (receiver == NetPacket::BROADCAST)
   {
      _total_packets_broadcasted ++;
      _total_flits_broadcasted += num_flits;
      _total_bytes_broadcasted += packet_length;
   }
}

void
NetworkModel::updateReceiveCounters(const NetPacket& packet, UInt64 zero_load_latency)
{
   tile_id_t sender = TILE_ID(packet.sender);
   tile_id_t receiver = TILE_ID(packet.receiver);

   tile_id_t requester = getRequester(packet);
   if ( (!_enabled) ||
        (requester >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) ||
        (sender == receiver) )
      return;

   assert( (receiver == NetPacket::BROADCAST) || (receiver == getNetwork()->getTile()->getId()) );

   UInt32 packet_length = getNetwork()->getModeledLength(packet);
   UInt32 num_flits = computeNumFlits(packet_length);

   _total_packets_received ++;
   _total_flits_received += num_flits;
   _total_bytes_received += packet_length;

   UInt64 packet_latency = packet.time - packet.start_time;
   UInt64 contention_delay = packet_latency - zero_load_latency;
   _total_packet_latency += packet_latency;
   _total_contention_delay += contention_delay;
}

void
NetworkModel::outputSummary(ostream& out)
{
   out << "    Total Packets Sent: " << _total_packets_sent << endl;
   out << "    Total Flits Sent: " << _total_flits_sent << endl;
   out << "    Total Bytes Sent: " << _total_bytes_sent << endl;

   out << "    Total Packets Broadcasted: " << _total_packets_broadcasted << endl;
   out << "    Total Flits Broadcasted: " << _total_flits_broadcasted << endl;
   out << "    Total Bytes Broadcasted: " << _total_bytes_broadcasted << endl;

   out << "    Total Packets Received: " << _total_packets_received << endl;
   out << "    Total Flits Received: " << _total_flits_received << endl;
   out << "    Total Bytes Received: " << _total_bytes_received << endl;

   if (_total_packets_received > 0)
   {
      UInt64 total_packet_latency_in_ns = convertCycleCount(_total_packet_latency, getFrequency(), 1.0);
      UInt64 total_contention_delay_in_ns = convertCycleCount(_total_contention_delay, getFrequency(), 1.0);

      out << "    Average Packet Latency (in clock cycles): " <<
         ((float) _total_packet_latency) / _total_packets_received << endl;
      out << "    Average Packet Latency (in ns): " <<
         ((float) total_packet_latency_in_ns) / _total_packets_received << endl;

      out << "    Average Contention Delay (in clock cycles): " <<
         ((float) _total_contention_delay) / _total_packets_received << endl;
      out << "    Average Contention Delay (in ns): " <<
         ((float) total_contention_delay_in_ns) / _total_packets_received << endl;
   }
   else
   {
      out << "    Average Packet Latency (in clock cycles): 0" << endl;
      out << "    Average Packet Latency (in ns): 0" << endl;
      
      out << "    Average Contention Delay (in clock cycles): 0" << endl;
      out << "    Average Contention Delay (in ns): 0" << endl;
   }
}

UInt32 
NetworkModel::parseNetworkType(string str)
{
   if (str == "magic")
      return NETWORK_MAGIC;
   else if (str == "emesh_hop_counter")
      return NETWORK_EMESH_HOP_COUNTER;
   else if (str == "analytical")
      return NETWORK_ANALYTICAL_MESH;
   else if (str == "emesh_hop_by_hop")
      return NETWORK_EMESH_HOP_BY_HOP;
   else if (str == "eclos")
      return NETWORK_ECLOS;
   else if (str == "atac")
      return NETWORK_ATAC;
   else
      return (UInt32)-1;
}

pair<bool,SInt32> 
NetworkModel::computeTileCountConstraints(UInt32 network_type, SInt32 tile_count)
{
   switch (network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ANALYTICAL_MESH:
         return make_pair(false,tile_count);

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::computeTileCountConstraints(tile_count);

      case NETWORK_ECLOS:
         return NetworkModelEClos::computeTileCountConstraints(tile_count);

      case NETWORK_ATAC:
         return NetworkModelAtac::computeTileCountConstraints(tile_count);
      
      default:
         fprintf(stderr, "Unrecognized network type(%u)\n", network_type);
         assert(false);
         return make_pair(false,-1);
   }
}

pair<bool, vector<tile_id_t> > 
NetworkModel::computeMemoryControllerPositions(UInt32 network_type, SInt32 num_memory_controllers, SInt32 tile_count)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_ECLOS:
         {
            SInt32 spacing_between_memory_controllers = tile_count / num_memory_controllers;
            vector<tile_id_t> tile_list_with_memory_controllers;
            for (tile_id_t i = 0; i < num_memory_controllers; i++)
            {
               assert((i*spacing_between_memory_controllers) < tile_count);
               tile_list_with_memory_controllers.push_back(i * spacing_between_memory_controllers);
            }
            
            return make_pair(false, tile_list_with_memory_controllers);
         }

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::computeMemoryControllerPositions(num_memory_controllers, tile_count);

      case NETWORK_ATAC:
         return NetworkModelAtac::computeMemoryControllerPositions(num_memory_controllers, tile_count);

      default:
         LOG_PRINT_ERROR("Unrecognized network type(%u)", network_type);
         return make_pair(false, vector<tile_id_t>());
   }
}

pair<bool, vector<Config::TileList> >
NetworkModel::computeProcessToTileMapping(UInt32 network_type)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_ANALYTICAL_MESH:
      case NETWORK_EMESH_HOP_COUNTER:
      case NETWORK_ECLOS:
         return make_pair(false, vector<vector<tile_id_t> >());

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::computeProcessToTileMapping();

      case NETWORK_ATAC:
         return NetworkModelAtac::computeProcessToTileMapping();

      default:
         fprintf(stderr, "Unrecognized network type(%u)\n", network_type);
         assert(false);
         return make_pair(false, vector<vector<tile_id_t> >());
   }
}
