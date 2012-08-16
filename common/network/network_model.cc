#include <cassert>
using namespace std;

#include "network.h"
#include "network_types.h"

#include "network_model_magic.h"
#include "network_model_emesh_hop_counter.h"
#include "network_model_emesh_hop_by_hop.h"
#include "network_model_atac.h"
#include "memory_manager.h"
#include "simulator.h"
#include "config.h"
#include "clock_converter.h"
#include "log.h"

NetworkModel::NetworkModel(Network *network, SInt32 network_id):
   _network(network),
   _network_id(network_id),
   _enabled(false)
{
   assert(network_id >= 0 && network_id < NUM_STATIC_NETWORKS);
   _network_name = g_static_network_name_list[network_id];

   // Get the Tile ID
   _tile_id = getNetwork()->getTile()->getId();
   // Get the Tile Width
   try
   {
      _tile_width = Sim()->getCfg()->getFloat("general/tile_width");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read tile_width from the cfg file");
   }

   // Initialize Event Counters
   initializeEventCounters();
   // Trace of Injection/Ejection Rate
   initializeCurrentUtilizationStatistics();
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

   case NETWORK_EMESH_HOP_BY_HOP:
      return new NetworkModelEMeshHopByHop(net, network_id);

   case NETWORK_ATAC:
      return new NetworkModelAtac(net, network_id);

   default:
      LOG_PRINT_ERROR("Unrecognized Network Model(%u)", model_type);
      return NULL;
   }
}

bool
NetworkModel::isPacketReadyToBeReceived(const NetPacket& pkt)
{
   if ( (_network_id >= STATIC_NETWORK_USER_1) && (_network_id <= STATIC_NETWORK_MEMORY_2) )
   {
      return (pkt.node_type == RECEIVE_TILE);
   }
   else
   {
      assert(TILE_ID(pkt.receiver) == _tile_id);
      return true;
   }
}

void
NetworkModel::__routePacket(const NetPacket& pkt, queue<Hop>& next_hops)
{
   ScopedLock sl(_lock);

   __attribute(__unused__) tile_id_t pkt_sender = TILE_ID(pkt.sender);
   __attribute(__unused__) tile_id_t pkt_receiver = TILE_ID(pkt.receiver);

   if (pkt.node_type == SEND_TILE)
   {
      LOG_ASSERT_ERROR(pkt_sender == _tile_id, "pkt_sender(%i), _tile_id(%i), name(%s)", pkt_sender, _tile_id, _network_name.c_str());
      bool processed = processCornerCases(pkt, next_hops);
      if (processed)
         return;

      if (isModelEnabled(pkt))
      {
         // Update Send Counters
         updateSendCounters(pkt);
      }
   }

   LOG_ASSERT_ERROR( isApplicationTile(pkt_sender)                                               &&
                     (isApplicationTile(pkt_receiver) || (pkt_receiver == NetPacket::BROADCAST)) &&
                     (pkt_sender != pkt_receiver),
                     "pkt_sender(%i), pkt_receiver(%i)", pkt_sender, pkt_receiver );

   // Call the routePacket() of the network model
   routePacket(pkt, next_hops);
}

void
NetworkModel::__processReceivedPacket(NetPacket& pkt)
{
   ScopedLock sl(_lock);

   tile_id_t pkt_sender = TILE_ID(pkt.sender);
   tile_id_t pkt_receiver = TILE_ID(pkt.receiver);
  
   if (_network_id != STATIC_NETWORK_SYSTEM)
      assert(pkt.node_type == RECEIVE_TILE);

   if ( (isSystemTile(pkt_sender))     ||
        (isSystemTile(_tile_id))       ||
        (pkt_sender == pkt_receiver)   ||
        (!isModelEnabled(pkt)) )
      return;

   // Do modifications of packet time
   processReceivedPacket(pkt);

   // Update Receive Counters
   updateReceiveCounters(pkt);
}

void
NetworkModel::processReceivedPacket(NetPacket& pkt)
{
   // Add serialization latency due to finite link bandwidth
   UInt64 num_flits = computeNumFlits(getModeledLength(pkt));

   pkt.time += num_flits;
   pkt.zero_load_delay += num_flits;
}

tile_id_t
NetworkModel::getRequester(const NetPacket& packet)
{
   __attribute(__unused__) SInt32 network_id = getNetworkId();
   LOG_ASSERT_ERROR((network_id == STATIC_NETWORK_MEMORY_1) || (network_id == STATIC_NETWORK_MEMORY_2),
                    "network_id(%i)", network_id);

   return getNetwork()->getTile()->getMemoryManager()->getShmemRequester(packet.data);
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

bool
NetworkModel::isModelEnabled(const NetPacket& pkt)
{
   SInt32 network_id = getNetworkId();
   if ((network_id == STATIC_NETWORK_MEMORY_1) || (network_id == STATIC_NETWORK_MEMORY_2))
   {
      return ( _enabled &&
               (!isSystemTile(getRequester(pkt))) &&
               (getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) );
   }
   else // USER_1, USER_2, SYSTEM
   {
      return _enabled;
   }
}

UInt32
NetworkModel::getModeledLength(const NetPacket& pkt)
{   
   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
   {
      // packet_type + sender + receiver + length + shmem_msg.size()
      // 1 byte for packet_type
      // log2(core_id) for sender and receiver
      // 2 bytes for packet length
      UInt32 metadata_size = 1 + 2 * Config::getSingleton()->getTileIDLength() + 2;
      UInt32 data_size = getNetwork()->getTile()->getCore()->getMemoryManager()->getModeledLength(pkt.data);
      return metadata_size + data_size;
   }
   else
   {
      return pkt.bufferSize();
   }
}

SInt32
NetworkModel::computeNumFlits(UInt32 packet_length)
{
   if (_flit_width == -1)
      return 0;

   SInt32 num_bits = packet_length * 8;
   if ( (num_bits % _flit_width) == 0 )
      return (num_bits / _flit_width);
   else
      return ( (num_bits / _flit_width) + 1 );
}

bool
NetworkModel::isApplicationTile(tile_id_t tile_id)
{
   return ( (0 <= tile_id) && (tile_id < (tile_id_t) Config::getSingleton()->getApplicationTiles()) );
}

// Is System Tile - Thread Spawner or MCP
bool
NetworkModel::isSystemTile(tile_id_t tile_id)
{
   return ( (tile_id >= (tile_id_t) Config::getSingleton()->getApplicationTiles()) &&
            (tile_id < (tile_id_t) Config::getSingleton()->getTotalTiles()) );
}

void
NetworkModel::updateSendCounters(const NetPacket& packet)
{
   __attribute(__unused__) tile_id_t sender = TILE_ID(packet.sender);
   tile_id_t receiver = TILE_ID(packet.receiver);

   LOG_ASSERT_ERROR(sender == _tile_id, "sender(%i), tile_id(%i)", sender, _tile_id);

   UInt32 packet_length = getModeledLength(packet);
   SInt32 num_flits = computeNumFlits(packet_length);
   
   _total_packets_sent ++;
   _total_flits_sent += num_flits;
   _total_bytes_sent += packet_length;
   _total_flits_sent_in_current_interval += num_flits;

   if (receiver == NetPacket::BROADCAST)
   {
      _total_packets_broadcasted ++;
      _total_flits_broadcasted += num_flits;
      _total_bytes_broadcasted += packet_length;
      _total_flits_broadcasted_in_current_interval += num_flits;
   }
}

void
NetworkModel::updateReceiveCounters(const NetPacket& packet)
{
   __attribute(__unused__) tile_id_t receiver = TILE_ID(packet.receiver);
   LOG_ASSERT_ERROR( (receiver == NetPacket::BROADCAST) || (receiver == _tile_id),
                     "receiver(%i), tile_id(%i)", receiver, _tile_id );
   
   UInt32 packet_length = getModeledLength(packet);
   SInt32 num_flits = computeNumFlits(packet_length);

   _total_packets_received ++;
   _total_flits_received += num_flits;
   _total_bytes_received += packet_length;
   _total_flits_received_in_current_interval += num_flits;

   UInt64 packet_latency = packet.zero_load_delay + packet.contention_delay;
   UInt64 contention_delay = packet.contention_delay;
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

bool
NetworkModel::isTileCountPermissible(UInt32 network_type, SInt32 tile_count)
{
   switch (network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
         return true;

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::isTileCountPermissible(tile_count);

      case NETWORK_ATAC:
         return NetworkModelAtac::isTileCountPermissible(tile_count);
      
      default:
         fprintf(stderr, "*ERROR* Unrecognized network type(%u)\n", network_type);
         abort();
         return false;
   }
}

pair<bool, vector<tile_id_t> > 
NetworkModel::computeMemoryControllerPositions(UInt32 network_type, SInt32 num_memory_controllers, SInt32 tile_count)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
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
         fprintf(stderr, "*ERROR* Unrecognized network type(%u)\n", network_type);
         abort();
         return make_pair(false, vector<tile_id_t>());
   }
}

pair<bool, vector<Config::TileList> >
NetworkModel::computeProcessToTileMapping(UInt32 network_type)
{
   switch(network_type)
   {
      case NETWORK_MAGIC:
      case NETWORK_EMESH_HOP_COUNTER:
         return make_pair(false, vector<vector<tile_id_t> >());

      case NETWORK_EMESH_HOP_BY_HOP:
         return NetworkModelEMeshHopByHop::computeProcessToTileMapping();

      case NETWORK_ATAC:
         return NetworkModelAtac::computeProcessToTileMapping();

      default:
         fprintf(stderr, "*ERROR* Unrecognized network type(%u)\n", network_type);
         abort();
         return make_pair(false, vector<vector<tile_id_t> >());
   }
}

bool
NetworkModel::processCornerCases(const NetPacket& pkt, queue<Hop>& next_hops)
{
   tile_id_t pkt_sender = TILE_ID(pkt.sender);
   tile_id_t pkt_receiver = TILE_ID(pkt.receiver);

   assert(pkt_sender == _tile_id);

   if (pkt_sender == pkt_receiver)
   {
      next_hops.push(Hop(pkt, pkt_receiver, RECEIVE_TILE));
   }

   else if (isSystemTile(pkt_sender))
   {
      if (pkt_receiver == NetPacket::BROADCAST)
      {
         for (tile_id_t i = 0; i < (tile_id_t) Config::getSingleton()->getTotalTiles(); i++)
         {
            next_hops.push(Hop(pkt, i, RECEIVE_TILE));
         }
      }
      else // (pkt_receiver != NetPacket::BROADCAST)
      {
         next_hops.push(Hop(pkt, pkt_receiver, RECEIVE_TILE));
      }
   }

   else if (isSystemTile(pkt_receiver))
   {
      next_hops.push(Hop(pkt, pkt_receiver, RECEIVE_TILE));
   }

   else
   {
      assert( (pkt_sender != pkt_receiver)                                                   &&
              isApplicationTile(pkt_sender)                                                  &&
              (isApplicationTile(pkt_receiver) || (pkt_receiver == NetPacket::BROADCAST)) );

      if (pkt_receiver == NetPacket::BROADCAST)
      {
         for (tile_id_t i = (tile_id_t) Config::getSingleton()->getApplicationTiles();
                        i < (tile_id_t) Config::getSingleton()->getTotalTiles();
                        i++)
         {
            next_hops.push(Hop(pkt, i, RECEIVE_TILE));
         }
      }

      // Only case where further processing is necessary
      return false;
   }

   // Completed processing corner cases
   return true;
}

void
NetworkModel::initializeCurrentUtilizationStatistics()
{
   _total_flits_sent_in_current_interval = 0;
   _total_flits_broadcasted_in_current_interval = 0;
   _total_flits_received_in_current_interval = 0;
}

void
NetworkModel::popCurrentUtilizationStatistics(UInt64& flits_sent, UInt64& flits_broadcasted, UInt64& flits_received)
{
   flits_sent = _total_flits_sent_in_current_interval;
   flits_broadcasted = _total_flits_broadcasted_in_current_interval;
   flits_received = _total_flits_received_in_current_interval;
   
   initializeCurrentUtilizationStatistics();
}

NetworkModel::Hop::Hop(const NetPacket& pkt, tile_id_t next_tile_id, SInt32 next_node_type,
                       UInt64 zero_load_delay, UInt64 contention_delay)
   : _next_tile_id(next_tile_id)
   , _next_node_type(next_node_type)
   , _time(pkt.time + contention_delay + zero_load_delay)
   , _zero_load_delay(pkt.zero_load_delay + zero_load_delay)
   , _contention_delay(pkt.contention_delay + contention_delay)
{}

NetworkModel::Hop::~Hop()
{}
