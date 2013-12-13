#include <cstring>
#include "transport.h"
#include "tile.h"
#include "core_model.h"
#include "network.h"
#include "memory_manager.h"
#include "simulator.h"
#include "tile_manager.h"
#include "network_model.h"
#include "core_model.h"
#include "statistics_manager.h"
#include "utils.h"
#include "log.h"

using namespace std;

// For getting a periodic summary of network utilization
bool* Network::_utilizationTraceEnabled;
ofstream* Network::_utilizationTraceFiles;

Network::Network(Tile *tile)
      : _tile(tile)
{
   LOG_ASSERT_ERROR(sizeof(g_type_to_static_network_map) / sizeof(EStaticNetwork) == NUM_PACKET_TYPES,
                    "Static network type map has incorrect number of entries.");

   _numMod = Config::getSingleton()->getTotalTiles();
   _tid = _tile->getId();

   _transport = Transport::getSingleton()->createNode(_tile->getId());

   _callbacks = new NetworkCallback [NUM_PACKET_TYPES];
   _callbackObjs = new void* [NUM_PACKET_TYPES];
   for (SInt32 i = 0; i < NUM_PACKET_TYPES; i++)
      _callbacks[i] = NULL;

   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      UInt32 network_model = NetworkModel::parseNetworkType(Config::getSingleton()->getNetworkType(i));
      _models[i] = NetworkModel::createModel(this, i, network_model);
   }

   // Shared Memory Shortcut enabled
   _sharedMemoryShortcutEnabled = Sim()->getCfg()->getBool("network/enable_shared_memory_shortcut", false);
   if (_sharedMemoryShortcutEnabled)
   {
      LOG_ASSERT_ERROR(Config::getSingleton()->getProcessCount() == 1,
                       "Cannot Enable Shared Memory Shortcut for (%i) processes", Config::getSingleton()->getProcessCount());
   }

   LOG_PRINT("Initialized Network.");
}

Network::~Network()
{
   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      delete _models[i];

   delete [] _callbackObjs;
   delete [] _callbacks;

   delete _transport;

   LOG_PRINT("Destroyed Network.");
}

void Network::registerCallback(PacketType type, NetworkCallback callback, void *obj)
{
   _callbacks[type] = callback;
   _callbackObjs[type] = obj;
}

void Network::unregisterCallback(PacketType type)
{
   assert((UInt32)type < NUM_PACKET_TYPES);
   _callbacks[type] = NULL;
}

void Network::outputSummary(std::ostream &out, const Time& target_completion_time) const
{
   out << "Network Summary: " << endl;
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      if (i >= STATIC_NETWORK_SYSTEM)
         break;         
      out << "  Network (" <<  _models[i]->getNetworkName() << "): " << endl;
      _models[i]->outputSummary(out, target_completion_time);
   }
}

// Polling function that performs background activities, such as
// pulling from the physical transport layer and routing packets to
// the appropriate queues.

void Network::netPullFromTransport()
{
   do
   {
      LOG_PRINT("Entering netPullFromTransport");

      NetPacket packet(_transport->recv());

      LOG_PRINT("Pull packet : type %i, from (%i, %i), time %llu",
                (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type, packet.time.toNanosec());
      LOG_ASSERT_ERROR(0 <= packet.sender.tile_id && packet.sender.tile_id < _numMod,
                       "Invalid Packet Sender(%i)", packet.sender);
      LOG_ASSERT_ERROR(0 <= packet.type && packet.type < NUM_PACKET_TYPES,
                       "Packet type: %d not between 0 and %d", packet.type, NUM_PACKET_TYPES);

      NetworkModel* model = getNetworkModelFromPacketType(packet.type);
   
      if (model->isPacketReadyToBeReceived(packet))   // Receive Packet
      {
         // I have accepted the packet - process the received packet
         model->__processReceivedPacket(packet);
         
         // asynchronous I/O support
         NetworkCallback callback = _callbacks[packet.type];

         if (callback != NULL)
         {
            LOG_PRINT("Executing callback on packet : type %i, from (%i, %i), to (%i, %i), tile_id %i, time %llu", 
                      (SInt32) packet.type, packet.sender.tile_id, packet.sender.core_type,
                      packet.receiver.tile_id, packet.receiver.core_type,
                      _tile->getId(), packet.time.toNanosec());
            assert(0 <= packet.sender.tile_id && packet.sender.tile_id < _numMod);
            assert(0 <= packet.type && packet.type < NUM_PACKET_TYPES);

            callback(_callbackObjs[packet.type], packet);

            // De-allocate packet payload
            if (packet.length > 0)
               delete [] (Byte*) packet.data;
         }

         // synchronous I/O support
         else
         {
            LOG_PRINT("Enqueuing packet : type %i, from (%i, %i), to (%i, %i), tile_id %i, time %llu",
                      (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type,
                      packet.receiver.tile_id, packet.receiver.core_type,
                      _tile->getId(), packet.time.toNanosec());

            _netQueueLock.acquire();
            _netQueue.push_back(packet);
            _netQueueLock.release();

            _netQueueCond.broadcast();
         }
      }

      else // Forward Packet
      { 
         LOG_PRINT("Forwarding packet : type %i, from (%i, %i), to (%i, %i), tile_id %i, time %llu.", 
                   (SInt32) packet.type, packet.sender.tile_id, packet.sender.core_type,
                   packet.receiver.tile_id, packet.receiver.core_type,
                   _tile->getId(), packet.time.toNanosec());

         forwardPacket(packet);
         
         // De-allocate packet payload
         if (packet.length > 0)
            delete [] (Byte*) packet.data;
      }
   }
   while (_transport->query());
}

NetworkModel* Network::getNetworkModelFromPacketType(PacketType packet_type)
{
   return _models[g_type_to_static_network_map[packet_type]];
}

SInt32 Network::netSend(NetPacket& packet)
{
   // Interface for sending packets on a network

   NetworkModel* model = getNetworkModelFromPacketType(packet.type);

   LOG_PRINT("netSend: type %i, from (%i,%i) to (%i,%i), tile_id %i, time %llu",
             packet.type, packet.sender.tile_id, packet.sender.core_type,
             packet.receiver.tile_id, packet.receiver.core_type,
             _tile->getId(), packet.time.toNanosec());
   

   // Send packet as multiple packets if model has not broadcast capability and receiver is ALL
   if ( (TILE_ID(packet.receiver) == NetPacket::BROADCAST) && (!model->hasBroadcastCapability()) )
   {
      for (tile_id_t i = 0; i < (tile_id_t) Config::getSingleton()->getTotalTiles(); i++)
      {
         packet.receiver = CORE_ID(i);
         __attribute__((unused)) SInt32 ret = forwardPacket(packet);
         LOG_ASSERT_ERROR(ret == (SInt32) packet.length, "forwardPacket-ret(%i) != packet.length(%u)", ret, packet.length);
      }
   }

   else // (packet.receiver != NetPacket::BROADCAST) || (model->hasBroadcastCapability())
   {
      __attribute__((unused)) SInt32 ret = forwardPacket(packet);
      LOG_ASSERT_ERROR(ret == (SInt32) packet.length, "forwardPacket-ret(%i) != packet.length(%u)", ret, packet.length);
   }

   return packet.length;
}

SInt32 Network::netSend(module_t module, NetPacket& packet)
{

   NetworkModel* model = getNetworkModelFromPacketType(packet.type);
   packet.time += model->getSynchronizationDelay(module);
   return netSend(packet);
}


SInt32 Network::forwardPacket(const NetPacket& packet)
{
   // Create a buffer suitable for forwarding
   Byte* buffer = packet.makeBuffer();
   NetPacket* buf_pkt = (NetPacket*) buffer;

   LOG_ASSERT_ERROR((buf_pkt->type >= 0) && (buf_pkt->type < NUM_PACKET_TYPES),
                    "buf_pkt->type(%u) INVALID", buf_pkt->type);

   NetworkModel *model = getNetworkModelFromPacketType(buf_pkt->type);

   queue<NetworkModel::Hop> hop_queue;
   model->__routePacket(*buf_pkt, hop_queue);

   while (!hop_queue.empty())
   {
      NetworkModel::Hop hop = hop_queue.front();
      hop_queue.pop();

      buf_pkt->node_type = hop._next_node_type;
      buf_pkt->time = hop._time;
      buf_pkt->zero_load_delay = hop._zero_load_delay;
      buf_pkt->contention_delay = hop._contention_delay;
      
      if ( (hop._next_node_type != NetworkModel::RECEIVE_TILE) && (_sharedMemoryShortcutEnabled) )
      {
         Tile* next_tile = Sim()->getTileManager()->getTileFromID(hop._next_tile_id);
         assert(next_tile);
         NetworkModel* next_network_model = next_tile->getNetwork()->getNetworkModelFromPacketType(buf_pkt->type);
         next_network_model->__routePacket(*buf_pkt, hop_queue);
      }
      else
      {
         LOG_PRINT("Send packet : type %i, from (%i,%i), to (%i, %i), next_hop %i, tile_id %i, time %llu",
                   (SInt32) buf_pkt->type,
                   buf_pkt->sender.tile_id, buf_pkt->sender.core_type,
                   buf_pkt->receiver.tile_id, buf_pkt->receiver.core_type,
                   hop._next_tile_id,
                   _tile->getId(), hop._time.toNanosec());
         
         _transport->send(hop._next_tile_id, buffer, packet.bufferSize());
      }
   }

   delete [] buffer;

   return packet.length;
}

// Stupid helper class to eliminate special cases for empty
// sender/type vectors in a NetMatch
class NetRecvIterator
{
   public:
      NetRecvIterator(UInt32 i)
            : _mode(INT)
            , _max(i)
            , _i(0)
      {
      }
      NetRecvIterator(const std::vector<core_id_t> &v)
            : _mode(SENDER_VECTOR)
            , _senders(&v)
            , _i(0)
      {
      }
      NetRecvIterator(const std::vector<PacketType> &v)
            : _mode(TYPE_VECTOR)
            , _types(&v)
            , _i(0)
      {
      }

      inline UInt32 get()
      {
         switch (_mode)
         {
         case INT:
            return _i;
         case TYPE_VECTOR:
            return (UInt32)_types->at(_i);
         default:
            assert(false);
            return (UInt32)-1;
         };
      }

      inline core_id_t getId()
      {
         switch (_mode)
         {
         case INT:
            return Tile::getMainCoreId(_i);
         case SENDER_VECTOR:
            return (core_id_t)_senders->at(_i);
         default:
            assert(false);
            return INVALID_CORE_ID;
         };
      }

      inline Boolean done()
      {
         switch (_mode)
         {
         case INT:
            return _i >= _max;
         case SENDER_VECTOR:
            return _i >= _senders->size();
         case TYPE_VECTOR:
            return _i >= _types->size();
         default:
            assert(false);
            return true;
         };
      }

      inline void next()
      {
         ++_i;
      }

      inline void reset()
      {
         _i = 0;
      }

   private:
      enum
      {
         INT, SENDER_VECTOR, TYPE_VECTOR
      } _mode;

      union
      {
         UInt32 _max;
         const std::vector<core_id_t> *_senders;
         const std::vector<PacketType> *_types;
      };

      UInt32 _i;
};

NetPacket Network::netRecv(const NetMatch &match)
{
   LOG_PRINT("netRecv: Entering.");

   // Track via iterator to minimize copying
   NetQueue::iterator itr;
   Boolean found;

   found = false;

   NetRecvIterator sender = match.senders.empty()
                            ? NetRecvIterator(_numMod)
                            : NetRecvIterator(match.senders);

   NetRecvIterator type = match.types.empty()
                          ? NetRecvIterator((UInt32)NUM_PACKET_TYPES)
                          : NetRecvIterator(match.types);

   core_id_t receiver = match.receiver.tile_id == INVALID_TILE_ID 
                        ? _tile->getCore()->getId() 
                        : match.receiver;

   LOG_ASSERT_ERROR(_tile && _tile->getCore()->getModel(),
                    "Tile and/or performance model not initialized.");
   Time start_time = _tile->getCore()->getModel()->getCurrTime();
   LOG_PRINT("netRecv: Start waiting at %llu", start_time.toNanosec());

   _netQueueLock.acquire();

   while (!found)
   {
      itr = _netQueue.end();

      // check every entry in the queue
      for (NetQueue::iterator i = _netQueue.begin();
            (i != _netQueue.end()) && !found;
            i++)
      {
         // make sure that this core is the proper destination core for this tile
         if (i->receiver.tile_id != receiver.tile_id || i->receiver.core_type != receiver.core_type)
         {
            if (i->receiver.tile_id != NetPacket::BROADCAST)
               continue;
         }

         // only find packets that match
         for (sender.reset(); !sender.done() && !found; sender.next())
         {
            if (i->sender.tile_id != sender.getId().tile_id || i->sender.core_type != sender.getId().core_type)
               continue;

            for (type.reset(); !type.done() && !found; type.next())
            {
               if (i->type != (PacketType)type.get())
                  continue;

               found = true;
               itr = i;
            }
         }
      }

      // go to sleep until a packet arrives if none have been found
      if (!found)
      {
         LOG_PRINT("netRecv: Packet match NOT found");
         LOG_PRINT("netRecv: Waiting on condition variable");
         _netQueueCond.wait(_netQueueLock);
         LOG_PRINT("netRecv: Woken up");
      }
   }

   LOG_PRINT("netRecv: Packet match found");

   assert(found == true && itr != _netQueue.end());
   assert(0 <= itr->sender.tile_id && itr->sender.tile_id < _numMod);
   assert(0 <= itr->type && itr->type < NUM_PACKET_TYPES);
   assert((itr->receiver.tile_id == _tile->getId()) || (itr->receiver.tile_id == NetPacket::BROADCAST));

   // Copy result
   NetPacket packet = *itr;
   _netQueue.erase(itr);

   _netQueueLock.release();

   LOG_PRINT("netRecv: Started waiting at %llu ns, Got packet at %llu ns", start_time.toNanosec(), packet.time.toNanosec());

   if (packet.time > start_time)
   {
      CoreModel* core_model = _tile->getCore()->getModel();
      if (core_model)
      {  
         LOG_PRINT("netRecv: Queueing RecvInstruction(%llu ns)", packet.time.toNanosec() - start_time.toNanosec());
         RecvInstruction *i = new RecvInstruction(packet.time - start_time);
         core_model->processDynamicInstruction(i);
      }
   }

   LOG_PRINT("netRecv: Exiting with type %i, from (%i,%i)",
             (SInt32) packet.type, packet.sender.tile_id, packet.sender.core_type);

   return packet;
}

// -- Wrappers

SInt32 Network::netSend(core_id_t dest, PacketType type, const void *buf, UInt32 len)
{
   LOG_PRINT("netSend: type %i to (%i,%i)", type, dest.tile_id, dest.core_type);
   NetPacket packet;
   assert(_tile);
   assert(_tile->getCore()->getModel()); 
   packet.time = _tile->getCore()->getModel()->getCurrTime();
   packet.sender = _tile->getCore()->getId();
   packet.receiver = dest;
   packet.length = len;
   packet.type = type;
   packet.data = buf;

   return netSend(packet);
}

SInt32 Network::netBroadcast(PacketType type, const void *buf, UInt32 len)
{
   LOG_PRINT("netBroadcast: type %i", type);
   core_id_t broadcast_receiver = (core_id_t) {NetPacket::BROADCAST, 0};
   return netSend(broadcast_receiver, type, buf, len);
}

NetPacket Network::netRecv(core_id_t src, core_id_t recv, PacketType type)
{
   LOG_PRINT("netRecv: type %i from (%i,%i)", type, src.tile_id, src.core_type);
   NetMatch match;
   match.senders.push_back(src);
   match.types.push_back(type);
   match.receiver = recv;
   return netRecv(match);
}

NetPacket Network::netRecvFrom(core_id_t src, core_id_t recv)
{
   LOG_PRINT("netRecvFrom: (%i,%i)", src.tile_id, src.core_type);
   NetMatch match;
   match.senders.push_back(src);
   match.receiver = recv;
   return netRecv(match);
}

NetPacket Network::netRecvType(PacketType type, core_id_t recv)
{
   LOG_PRINT("netRecvType: %i", type);
   NetMatch match;
   match.types.push_back(type);
   match.receiver = recv;
   return netRecv(match);
}

void Network::enableModels()
{
   LOG_PRINT("enableModels: (%i) start", _tile->getId());
   for (int i = 0; i < NUM_STATIC_NETWORKS; i++)
      _models[i]->enable();
   LOG_PRINT("enableModels: (%i) end", _tile->getId());
}

void Network::disableModels()
{
   LOG_PRINT("disableModels: (%i) start", _tile->getId());
   for (int i = 0; i < NUM_STATIC_NETWORKS; i++)
      _models[i]->disable();
   LOG_PRINT("disableModels: (%i) end", _tile->getId());
}

// Get a Trace of Network Traffic
// Works only on a single process currently

void Network::openUtilizationTraceFiles()
{
   // Create the TraceEnabled and TraceFiles array
   _utilizationTraceEnabled = new bool[NUM_STATIC_NETWORKS];
   _utilizationTraceFiles = new ofstream[NUM_STATIC_NETWORKS];

   // Populate _network_traffic_trace_enabled with the networks for which tracing is enabled 
   computeTraceEnabledNetworks();

   // Open the trace files 
   for (SInt32 network_id = 0; network_id < NUM_STATIC_NETWORKS; network_id ++)
   {
      if (_utilizationTraceEnabled[network_id])
      {
         string output_dir;
         try
         {
            output_dir = Sim()->getCfg()->getString("general/output_dir");
         }
         catch (...)
         {
            LOG_PRINT_ERROR("Could not read general/output_dir from the cfg file");
         }

         string filename = output_dir + "/network_utilization_" + g_static_network_name_list[network_id] + ".dat";
         _utilizationTraceFiles[network_id].open(filename.c_str());
      }
   }
}

void Network::computeTraceEnabledNetworks()
{
   // Is tracing enabled for the individual networks
   for (SInt32 network_id = 0; network_id < NUM_STATIC_NETWORKS; network_id ++)
      _utilizationTraceEnabled[network_id] = false;

   string enabled_networks_line;
   try
   {
      enabled_networks_line = Sim()->getCfg()->getString("statistics_trace/network_utilization/enabled_networks");
   }
   catch (...)
   {
      LOG_PRINT_ERROR("Could not read statistics_trace/network/enabled_networks from the cfg file");
   }
 
   vector<string> enabled_networks; 
   splitIntoTokens(enabled_networks_line, enabled_networks, ", "); 
   for (vector<string>::iterator it = enabled_networks.begin(); it != enabled_networks.end(); it ++)
   {
      string network_name = *it;
      for (SInt32 network_id = 0; network_id < NUM_STATIC_NETWORKS; network_id ++)
      {
         if (g_static_network_name_list[network_id] == network_name)
            _utilizationTraceEnabled[network_id] = true;
      }
   }
}

void Network::closeUtilizationTraceFiles()
{
   for (SInt32 network_id = 0; network_id < NUM_STATIC_NETWORKS; network_id ++)
   {
      if (_utilizationTraceEnabled[network_id])
      {
         _utilizationTraceFiles[network_id].close();
      }
   }

   // Delete the TraceFiles and TraceEnabled arrays
   delete [] _utilizationTraceFiles;
   delete [] _utilizationTraceEnabled;
}

void Network::outputUtilizationSummary()
{
   for (SInt32 network_id = 0; network_id < NUM_STATIC_NETWORKS; network_id ++)
   {
      if (_utilizationTraceEnabled[network_id])
      {
         UInt64 total_flits_sent = 0;
         UInt64 total_flits_broadcasted = 0;
         UInt64 total_flits_received = 0;

         SInt32 total_tiles = (SInt32) Config::getSingleton()->getTotalTiles();
         SInt32 sampling_interval = Sim()->getStatisticsManager()->getSamplingInterval();

         for (SInt32 tile_id = 0; tile_id < total_tiles; tile_id ++)
         {
            Tile* tile = Sim()->getTileManager()->getTileFromID(tile_id);
            assert(tile);
            NetworkModel* network_model = tile->getNetwork()->getNetworkModel(network_id);
           
            UInt64 flits_sent, flits_broadcasted, flits_received;
            network_model->popCurrentUtilizationStatistics(flits_sent, flits_broadcasted, flits_received);
            
            total_flits_sent += flits_sent;
            total_flits_broadcasted += flits_broadcasted;
            total_flits_received += flits_received;
         }
           
         double flits_send_rate = ((double) total_flits_sent) / (total_tiles * sampling_interval);
         double flits_broadcast_rate = ((double) total_flits_broadcasted) / (total_tiles * sampling_interval);
         double flits_receive_rate = ((double) total_flits_received) / (total_tiles * sampling_interval);
         _utilizationTraceFiles[network_id] << flits_send_rate << ", " << flits_broadcast_rate << ", " << flits_receive_rate << endl;
      }
   }
}

// -- NetPacket

NetPacket::NetPacket()
   : time(0)
   , type(INVALID_PACKET_TYPE)
   , sender(INVALID_CORE_ID)
   , receiver(INVALID_CORE_ID)
   , node_type(NetworkModel::SEND_TILE)
   , length(0)
   , data(0)
   , zero_load_delay(0)
   , contention_delay(0)
{
}

NetPacket::NetPacket(Time t, PacketType ty, SInt32 s,
                     SInt32 r, UInt32 l, const void *d)
   : time(t)
   , type(ty)
   , node_type(NetworkModel::SEND_TILE)
   , length(l)
   , data(d)
   , zero_load_delay(0)
   , contention_delay(0)
{
   sender = Tile::getMainCoreId(s);
   receiver = Tile::getMainCoreId(r);
}


NetPacket::NetPacket(Time t, PacketType ty, core_id_t s,
                     core_id_t r, UInt32 l, const void *d)
   : time(t)
   , type(ty)
   , sender(s)
   , receiver(r)
   , node_type(NetworkModel::SEND_TILE)
   , length(l)
   , data(d)
   , zero_load_delay(0)
   , contention_delay(0)
{
}

NetPacket::NetPacket(Byte *buffer)
{
   memcpy(this, buffer, sizeof(*this));

   // LOG_ASSERT_ERROR(length > 0, "type(%u), sender(%i), receiver(%i), length(%u)", type, sender, receiver, length);
   if (length > 0)
   {
      Byte* data_buffer = new Byte[length];
      memcpy(data_buffer, buffer + sizeof(*this), length);
      data = data_buffer;
   }

   delete [] buffer;
}

// This implementation is slightly wasteful because there is no need
// to copy the const void* value in the NetPacket when length == 0,
// but I don't see this as a major issue.
UInt32 NetPacket::bufferSize() const
{
   return (sizeof(*this) + length);
}

Byte* NetPacket::makeBuffer() const
{
   UInt32 size = bufferSize();
   assert(size >= sizeof(NetPacket));

   Byte *buffer = new Byte[size];

   memcpy(buffer, this, sizeof(*this));
   memcpy(buffer + sizeof(*this), data, length);

   return buffer;
}
