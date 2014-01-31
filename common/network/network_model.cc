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
#include "log.h"
#include "dvfs_manager.h"

NetworkModel::NetworkModel(Network *network, SInt32 network_id)
   : _frequency(0)
   , _voltage(0)
   , _module(INVALID_MODULE)
   , _network(network)
   , _network_id(network_id)
   , _enabled(false)
{
   assert(network_id >= 0 && network_id < NUM_STATIC_NETWORKS);
   _network_name = g_static_network_name_list[network_id];

   // Initialize DVFS
   initializeDVFS();

   // Get the Tile ID
   _tile_id = _network->getTile()->getId();
   
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
   if ( (_network_id == STATIC_NETWORK_USER) || (_network_id == STATIC_NETWORK_MEMORY) )
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

   __attribute__((unused)) tile_id_t pkt_sender = TILE_ID(pkt.sender);
   __attribute__((unused)) tile_id_t pkt_receiver = TILE_ID(pkt.receiver);

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

   pkt.time += Latency(num_flits,_frequency);
   pkt.zero_load_delay += Latency(num_flits,_frequency);
}

void
NetworkModel::initializeEventCounters()
{
   _total_packets_sent = 0;
   _total_flits_sent = 0;
   _total_bits_sent = 0;
   
   _total_packets_broadcasted = 0;
   _total_flits_broadcasted = 0;
   _total_bits_broadcasted = 0;
   
   _total_packets_received = 0;
   _total_flits_received = 0;
   _total_bits_received = 0;
   
   _total_packet_latency = Time(0);
   _total_contention_delay = Time(0);
}

bool
NetworkModel::isModelEnabled(const NetPacket& pkt)
{
   SInt32 network_id = getNetworkID();
   if (network_id == STATIC_NETWORK_MEMORY)
   {
      return ( _enabled && (getNetwork()->getTile()->getMemoryManager()->isModeled(pkt.data)) );
   }
   else // USER, SYSTEM
   {
      return _enabled;
   }
}

UInt32
NetworkModel::getModeledLength(const NetPacket& pkt) // In bits
{   
   if (pkt.type == SHARED_MEM)
   {
      // sender + receiver + size of shmem_msg
      // log2(core_id) for sender and receiver
      UInt32 metadata_size = 2 * Config::getSingleton()->getTileIDLength();
      UInt32 data_size = getNetwork()->getTile()->getMemoryManager()->getModeledLength(pkt.data);
      return metadata_size + data_size;
   }
   else
   {
      return (pkt.bufferSize() * 8);
   }
}

SInt32
NetworkModel::computeNumFlits(UInt32 packet_length)
{
   if (_flit_width == -1)
      return 0;

   if ( (packet_length % _flit_width) == 0 )
      return (packet_length / _flit_width);
   else
      return ( (packet_length / _flit_width) + 1 );
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
   __attribute__((unused)) tile_id_t sender = TILE_ID(packet.sender);
   tile_id_t receiver = TILE_ID(packet.receiver);

   LOG_ASSERT_ERROR(sender == _tile_id, "sender(%i), tile_id(%i)", sender, _tile_id);

   UInt32 packet_length = getModeledLength(packet); // In bits
   SInt32 num_flits = computeNumFlits(packet_length);
   
   _total_packets_sent ++;
   _total_flits_sent += num_flits;
   _total_bits_sent += packet_length;
   _total_flits_sent_in_current_interval += num_flits;

   if (receiver == NetPacket::BROADCAST)
   {
      _total_packets_broadcasted ++;
      _total_flits_broadcasted += num_flits;
      _total_bits_broadcasted += packet_length;
      _total_flits_broadcasted_in_current_interval += num_flits;
   }
}

void
NetworkModel::updateReceiveCounters(const NetPacket& packet)
{
   __attribute__((unused)) tile_id_t receiver = TILE_ID(packet.receiver);
   LOG_ASSERT_ERROR( (receiver == NetPacket::BROADCAST) || (receiver == _tile_id),
                     "receiver(%i), tile_id(%i)", receiver, _tile_id );
   
   UInt32 packet_length = getModeledLength(packet); // In bits
   SInt32 num_flits = computeNumFlits(packet_length);

   _total_packets_received ++;
   _total_flits_received += num_flits;
   _total_bits_received += packet_length;
   _total_flits_received_in_current_interval += num_flits;

   Time packet_latency = packet.zero_load_delay + packet.contention_delay;
   Time contention_delay = packet.contention_delay;
   _total_packet_latency += packet_latency;
   _total_contention_delay += contention_delay;
}

void
NetworkModel::outputSummary(ostream& out, const Time& target_completion_time)
{
   out << "    Total Packets Sent: " << _total_packets_sent << endl;
   out << "    Total Flits Sent: " << _total_flits_sent << endl;
   out << "    Total Bits Sent: " << _total_bits_sent << endl;

   out << "    Total Packets Broadcasted: " << _total_packets_broadcasted << endl;
   out << "    Total Flits Broadcasted: " << _total_flits_broadcasted << endl;
   out << "    Total Bits Broadcasted: " << _total_bits_broadcasted << endl;

   out << "    Total Packets Received: " << _total_packets_received << endl;
   out << "    Total Flits Received: " << _total_flits_received << endl;
   out << "    Total Bits Received: " << _total_bits_received << endl;

   if (_total_packets_received > 0)
   {
      UInt64 total_packet_latency_in_ns = _total_packet_latency.toNanosec();
      UInt64 total_contention_delay_in_ns = _total_contention_delay.toNanosec();

      out << "    Average Packet Latency (in clock cycles): " <<
         ((float) _total_packet_latency.toCycles(_frequency)) / _total_packets_received << endl;
      out << "    Average Packet Latency (in nanoseconds): " <<
         ((float) total_packet_latency_in_ns) / _total_packets_received << endl;

      out << "    Average Contention Delay (in clock cycles): " <<
         ((float) _total_contention_delay.toCycles(_frequency)) / _total_packets_received << endl;
      out << "    Average Contention Delay (in nanoseconds): " <<
         ((float) total_contention_delay_in_ns) / _total_packets_received << endl;
   }
   else
   {
      out << "    Average Packet Latency (in clock cycles): 0" << endl;
      out << "    Average Packet Latency (in nanoseconds): 0" << endl;
      
      out << "    Average Contention Delay (in clock cycles): 0" << endl;
      out << "    Average Contention Delay (in nanoseconds): 0" << endl;
   }
   
   // Asynchronous communication
   if (_module != INVALID_MODULE)
      DVFSManager::printAsynchronousMap(out, _module, _asynchronous_map);
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

void NetworkModel::initializeDVFS()
{
   // Initialize frequency, voltage
   switch (_network_id)
   {
      case STATIC_NETWORK_USER:
         _module = NETWORK_USER;
         break;
      case STATIC_NETWORK_MEMORY:
         _module = NETWORK_MEMORY;
         break;
      default:
         _module = INVALID_MODULE;
         break;
   }

   int rc = DVFSManager::getInitialFrequencyAndVoltage(_module, _frequency, _voltage);
   LOG_ASSERT_ERROR(rc == 0, "Error setting initial voltage for frequency(%g)", _frequency);

   // Asynchronous communication
   _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), _frequency));

   // Asynchronous communication
   _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), _frequency));
   _asynchronous_map[L2_CACHE] = Time(0);
   if (MemoryManager::getCachingProtocolType() == PR_L1_SH_L2_MSI)
   {
      _asynchronous_map[L1_ICACHE] = Time(0);
      _asynchronous_map[L1_DCACHE] = Time(0);
   }
   else
   {
      _asynchronous_map[DIRECTORY] = Time(0);
   }
}

int
NetworkModel::getDVFS(double &frequency, double &voltage)
{
   frequency = _frequency;
   voltage = _voltage;
   return 0;
}

int
NetworkModel::setDVFS(double frequency, voltage_option_t voltage_flag, const Time& curr_time)
{
   // Get voltage at new frequency
   int rc = DVFSManager::getVoltage(_voltage, voltage_flag, frequency);
   if (rc==0)
   {
      _frequency = frequency;
      setDVFS(_frequency, _voltage, curr_time);
      _synchronization_delay = Time(Latency(DVFSManager::getSynchronizationDelay(), _frequency));
   }
   return rc;
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

Time
NetworkModel::getSynchronizationDelay(module_t module)
{
   if (!DVFSManager::hasSameDVFSDomain(_module, module) && _enabled){
      _asynchronous_map[module] += _synchronization_delay;
      return _synchronization_delay;
   }
   return Time(0);
}

NetworkModel::Hop::Hop(const NetPacket& pkt, tile_id_t next_tile_id, SInt32 next_node_type,
                       Time zero_load_delay, Time contention_delay)
   : _next_tile_id(next_tile_id)
   , _next_node_type(next_node_type)
   , _time(pkt.time + contention_delay + zero_load_delay)
   , _zero_load_delay(pkt.zero_load_delay + zero_load_delay)
   , _contention_delay(pkt.contention_delay + contention_delay)
{}

NetworkModel::Hop::~Hop()
{}

