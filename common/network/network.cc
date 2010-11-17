#include <string.h>

#include "transport.h"
#include "tile.h"
#include "network.h"
#include "memory_manager_base.h"
#include "simulator.h"
#include "tile_manager.h"
#include "clock_converter.h"
#include "fxsupport.h"
#include "log.h"

using namespace std;

// FIXME: Rework netCreateBuf and netExPacket. We don't need to
// duplicate the sender/receiver info the packet. This should be known
// by the transport layer and given to us. We also should be more
// intelligent about the time stamps, right now the method is very
// ugly.

Network::Network(Tile *tile)
      : _tile(tile)
{
   LOG_ASSERT_ERROR(sizeof(g_type_to_static_network_map) / sizeof(EStaticNetwork) == NUM_PACKET_TYPES,
                    "Static network type map has incorrect number of entries.");

   _numMod = Config::getSingleton()->getTotalCores();
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

   LOG_PRINT("Initialized.");
}

Network::~Network()
{
   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      delete _models[i];

   delete [] _callbackObjs;
   delete [] _callbacks;

   delete _transport;

   LOG_PRINT("Destroyed.");
}

void Network::registerCallback(PacketType type, NetworkCallback callback, void *obj)
{
   assert((UInt32)type < NUM_PACKET_TYPES);
   _callbacks[type] = callback;
   _callbackObjs[type] = obj;
}

void Network::unregisterCallback(PacketType type)
{
   assert((UInt32)type < NUM_PACKET_TYPES);
   _callbacks[type] = NULL;
}

void Network::outputSummary(std::ostream &out) const
{
   out << "Network summary:\n";
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      out << "  Network model " << i << ":\n";
      _models[i]->outputSummary(out);
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

      LOG_PRINT("Pull packet : type %i, from %i, time %llu", (SInt32)packet.type, packet.sender, packet.time);
      LOG_ASSERT_ERROR(0 <= packet.sender && packet.sender < _numMod,
            "Invalid Packet Sender(%i)", packet.sender);
      LOG_ASSERT_ERROR(0 <= packet.type && packet.type < NUM_PACKET_TYPES,
            "Packet type: %d not between 0 and %d", packet.type, NUM_PACKET_TYPES);

      NetworkModel* model = getNetworkModelFromPacketType(packet.type);
     
      UInt32 action = model->computeAction(packet);
      
      if (action & NetworkModel::RoutingAction::FORWARD)
      {
         LOG_PRINT("Forwarding packet : type %i, from %i, to %i, core_id %i, time %llu.", 
               (SInt32)packet.type, packet.sender, packet.receiver, _tile->getId(), packet.time);
         forwardPacket(packet);
      }
      
      if (action & NetworkModel::RoutingAction::RECEIVE)
      {
         LOG_PRINT("Before Processing Received Packet: packet.time(%llu)", packet.time);
         
         // I have accepted the packet - process the received packet
         model->processReceivedPacket(packet);

         LOG_PRINT("After Processing Received Packet: packet.time(%llu)", packet.time);
         
         // Assume that the network is used by the main core for now, until I had packet types
         // for the PEP core.
         // Convert from network cycle count to core cycle count
         packet.time = convertCycleCount(packet.time, \
               getNetworkModelFromPacketType(packet.type)->getFrequency(), \
               _tile->getCore()->getPerformanceModel()->getFrequency());
    
         LOG_PRINT("After Converting Cycle Count: packet.time(%llu)", packet.time);
         
         // asynchronous I/O support
         NetworkCallback callback = _callbacks[packet.type];

         if (callback != NULL)
         {
            LOG_PRINT("Executing callback on packet : type %i, from %i, to %i, core_id %i, cycle_count %llu", 
                  (SInt32)packet.type, packet.sender, packet.receiver, _tile->getId(), packet.time);
            assert(0 <= packet.sender && packet.sender < _numMod);
            assert(0 <= packet.type && packet.type < NUM_PACKET_TYPES);

            callback(_callbackObjs[packet.type], packet);

            if (packet.length > 0)
               delete [] (Byte*) packet.data;
         }

         // synchronous I/O support
         else
         {
            LOG_PRINT("Enqueuing packet : type %i, from %i, to %i, core_id %i, cycle_count %llu", 
                  (SInt32)packet.type, packet.sender, packet.receiver, _tile->getId(), packet.time);
            _netQueueLock.acquire();
            _netQueue.push_back(packet);
            _netQueueLock.release();
            _netQueueCond.broadcast();
         }
      }
      else // if ((action & NetworkModel::RoutingAction::RECEIVE) == 0)
      {
         if (packet.length > 0)
            delete [] (Byte*) packet.data;
      }
   }
   while (_transport->query());
}

SInt32 Network::forwardPacket(const NetPacket& packet)
{
   // Create a buffer suitable for forwarding
   Byte* buffer = packet.makeBuffer();
   NetPacket* buff_pkt = (NetPacket*) buffer;

   LOG_ASSERT_ERROR((buff_pkt->type >= 0) && (buff_pkt->type < NUM_PACKET_TYPES),
         "buff_pkt->type(%u)", buff_pkt->type);

   NetworkModel *model = getNetworkModelFromPacketType(buff_pkt->type);

   vector<NetworkModel::Hop> hopVec;
   model->routePacket(*buff_pkt, hopVec);

   for (UInt32 i = 0; i < hopVec.size(); i++)
   {
      LOG_PRINT("Send packet : type %i, from %i, to %i, next_hop %i, core_id %i, time %llu", \
            (SInt32) buff_pkt->type, buff_pkt->sender, hopVec[i].final_dest, hopVec[i].next_dest, \
            _tile->getId(), hopVec[i].time);

      // Do a shared memory shortcut here
      if ((Config::getSingleton()->getProcessCount() == 1) && (hopVec[i].final_dest != NetPacket::BROADCAST))
      {
         // 1) Process Count = 1
         // 2) The broadcast tree network model is not used
         while (1)
         {
            buff_pkt->time = hopVec[i].time;
            buff_pkt->receiver = hopVec[i].final_dest;
            buff_pkt->specific = hopVec[i].specific;

            Tile* remote_tile = Sim()->getTileManager()->getTileFromID(hopVec[i].next_dest);
            NetworkModel* remote_network_model = remote_tile->getNetwork()->getNetworkModelFromPacketType(buff_pkt->type);

            UInt32 action = remote_network_model->computeAction(*buff_pkt);
            LOG_ASSERT_ERROR(!((action & NetworkModel::RoutingAction::RECEIVE) && \
                     (action & NetworkModel::RoutingAction::FORWARD)), "action(%u)", action);
            
            if (action & NetworkModel::RoutingAction::RECEIVE)
            {
               // Destination reached. Break out of the routing loop
               break;
            }
            
            vector<NetworkModel::Hop> localHopVec;
            remote_network_model->routePacket(*buff_pkt, localHopVec);
            LOG_ASSERT_ERROR(localHopVec.size() == 1, "Only unicasts allowed in this routing loop(%u)",
                  localHopVec.size());

            hopVec[i] = localHopVec[0];
         }
      }

      buff_pkt->time = hopVec[i].time;
      buff_pkt->receiver = hopVec[i].final_dest;
      buff_pkt->specific = hopVec[i].specific;

      _transport->send(hopVec[i].next_dest, buffer, packet.bufferSize());
      
      LOG_PRINT("Sent packet");
   }

   delete [] buffer;

   return packet.length;
}

NetworkModel* Network::getNetworkModelFromPacketType(PacketType packet_type)
{
   return _models[g_type_to_static_network_map[packet_type]];
}

SInt32 Network::netSend(NetPacket& packet)
{
   // Interface for sending packets on a network

   // Floating Point Save/Restore
   FloatingPointHandler floating_point_handler;

   // Convert from core cycle count to network cycle count
   packet.time = convertCycleCount(packet.time, \
         _tile->getCore()->getPerformanceModel()->getFrequency(), \
         getNetworkModelFromPacketType(packet.type)->getFrequency());

   // Note the start time
   packet.start_time = packet.time;

   // Call forwardPacket(packet)
   return forwardPacket(packet);
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
      NetRecvIterator(const std::vector<SInt32> &v)
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
         case SENDER_VECTOR:
            return (UInt32)_senders->at(_i);
         case TYPE_VECTOR:
            return (UInt32)_types->at(_i);
         default:
            assert(false);
            return (UInt32)-1;
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
         const std::vector<SInt32> *_senders;
         const std::vector<PacketType> *_types;
      };

      UInt32 _i;
};

NetPacket Network::netRecv(const NetMatch &match)
{
   LOG_PRINT("Entering netRecv.");

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

   LOG_ASSERT_ERROR(_tile && _tile->getCore()->getPerformanceModel(),
                    "Tile and/or performance model not initialized.");
   UInt64 start_time = _tile->getCore()->getPerformanceModel()->getCycleCount();

   _netQueueLock.acquire();

   while (!found)
   {
      itr = _netQueue.end();

      // check every entry in the queue
      for (NetQueue::iterator i = _netQueue.begin();
            i != _netQueue.end();
            i++)
      {
         // only find packets that match
         for (sender.reset(); !sender.done(); sender.next())
         {
            if (i->sender != (SInt32)sender.get())
               continue;

            for (type.reset(); !type.done(); type.next())
            {
               if (i->type != (PacketType)type.get())
                  continue;

               found = true;

               // find the earliest packet
               if (itr == _netQueue.end() ||
                   itr->time > i->time)
               {
                  itr = i;
               }
            }
         }
      }

      // go to sleep until a packet arrives if none have been found
      if (!found)
      {
         _netQueueCond.wait(_netQueueLock);
      }
   }

   assert(found == true && itr != _netQueue.end());
   assert(0 <= itr->sender && itr->sender < _numMod);
   assert(0 <= itr->type && itr->type < NUM_PACKET_TYPES);
   assert((itr->receiver == _tile->getId()) || (itr->receiver == NetPacket::BROADCAST));

   // Copy result
   NetPacket packet = *itr;
   _netQueue.erase(itr);
   _netQueueLock.release();

   LOG_PRINT("packet.time(%llu), start_time(%llu)", packet.time, start_time);

   if (packet.time > start_time)
   {
      LOG_PRINT("Queueing RecvInstruction(%llu)", packet.time - start_time);
      Instruction *i = new RecvInstruction(packet.time - start_time);
      _tile->getCore()->getPerformanceModel()->queueDynamicInstruction(i);
   }

   LOG_PRINT("Exiting netRecv : type %i, from %i", (SInt32)packet.type, packet.sender);

   return packet;
}

// -- Wrappers

SInt32 Network::netSend(SInt32 dest, PacketType type, const void *buf, UInt32 len)
{
   NetPacket packet;
   assert(_tile && _tile->getCore()->getPerformanceModel());
   packet.time = _tile->getCore()->getPerformanceModel()->getCycleCount();
   packet.sender = _tile->getId();
   packet.receiver = dest;
   packet.length = len;
   packet.type = type;
   packet.data = buf;

   return netSend(packet);
}

SInt32 Network::netBroadcast(PacketType type, const void *buf, UInt32 len)
{
   return netSend(NetPacket::BROADCAST, type, buf, len);
}

NetPacket Network::netRecv(SInt32 src, PacketType type)
{
   NetMatch match;
   match.senders.push_back(src);
   match.types.push_back(type);
   return netRecv(match);
}

NetPacket Network::netRecvFrom(SInt32 src)
{
   NetMatch match;
   match.senders.push_back(src);
   return netRecv(match);
}

NetPacket Network::netRecvType(PacketType type)
{
   NetMatch match;
   match.types.push_back(type);
   return netRecv(match);
}

void Network::enableModels()
{
   for (int i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _models[i]->enable();
   }
}

void Network::disableModels()
{
   for (int i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _models[i]->disable();
   }
}

// Modeling
UInt32 Network::getModeledLength(const NetPacket& pkt)
{
   if ((pkt.type == SHARED_MEM_1) || (pkt.type == SHARED_MEM_2))
   {
      // packet_type + sender + receiver + length + shmem_msg.size()
      // 1 byte for packet_type
      // log2(core_id) for sender and receiver
      // 2 bytes for packet length
      UInt32 metadata_size = 1 + 2 * Config::getSingleton()->getCoreIDLength() + 2;
      UInt32 data_size = getTile()->getCore()->getMemoryManager()->getModeledLength(pkt.data);
      return metadata_size + data_size;
   }
   else
   {
      return pkt.bufferSize();
   }
}

// -- NetPacket

NetPacket::NetPacket()
   : start_time(0)
   , time(0)
   , type(INVALID_PACKET_TYPE)
   , sender(INVALID_CORE_ID)
   , receiver(INVALID_CORE_ID)
   , specific(0)
   , length(0)
   , data(0)
{
}

NetPacket::NetPacket(UInt64 t, PacketType ty, SInt32 s,
                     SInt32 r, UInt32 l, const void *d)
   : start_time(0)
   , time(t)
   , type(ty)
   , sender(s)
   , receiver(r)
   , specific(0)
   , length(l)
   , data(d)
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
