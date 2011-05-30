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

      LOG_PRINT("Pull packet : type %i, from {%i, %i}, time %llu", (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type, packet.time);
      LOG_ASSERT_ERROR(0 <= packet.sender.tile_id && packet.sender.tile_id < _numMod,
            "Invalid Packet Sender(%i)", packet.sender);
      LOG_ASSERT_ERROR(0 <= packet.type && packet.type < NUM_PACKET_TYPES,
            "Packet type: %d not between 0 and %d", packet.type, NUM_PACKET_TYPES);

      NetworkModel* model = getNetworkModelFromPacketType(packet.type);
     
      UInt32 action = model->computeAction(packet);
      
      if (action & NetworkModel::RoutingAction::FORWARD)
      {
         LOG_PRINT("Forwarding packet : type %i, from {%i, %i}, to {%i, %i}, tile_id %i, time %llu.", 
               (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type, _tile->getId(), packet.time);
         forwardPacket(packet);
      }
      
      if (action & NetworkModel::RoutingAction::RECEIVE)
      {
         LOG_PRINT("Before Processing Received Packet: packet.time(%llu)", packet.time);
         
         // I have accepted the packet - process the received packet
         model->processReceivedPacket(packet);

         LOG_PRINT("After Processing Received Packet: packet.time(%llu)", packet.time);
         
         // Convert from network cycle count to core cycle count
         packet.time = convertCycleCount(packet.time,
               getNetworkModelFromPacketType(packet.type)->getFrequency(),
               _tile->getCore()->getPerformanceModel()->getFrequency());
    
         LOG_PRINT("After Converting Cycle Count: packet.time(%llu)", packet.time);
         
         // asynchronous I/O support
         NetworkCallback callback = _callbacks[packet.type];

         if (callback != NULL)
         {
            LOG_PRINT("Executing callback on packet : type %i, from {%i, %i}, to {%i, %i}, tile_id %i, cycle_count %llu", 
                  (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type, _tile->getId(), packet.time);
            assert(0 <= packet.sender.tile_id && packet.sender.tile_id < _numMod);
            assert(0 <= packet.type && packet.type < NUM_PACKET_TYPES);

            callback(_callbackObjs[packet.type], packet);

            if (packet.length > 0)
               delete [] (Byte*) packet.data;
         }

         // synchronous I/O support
         else
         {
            LOG_PRINT("Enqueuing packet : type %i, from {%i, %i}, to {%i, %i}, core_id %i, cycle_count %llu", 
                  (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type, packet.receiver.tile_id, packet.receiver.core_type, _tile->getId(), packet.time);

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
      LOG_PRINT("Send packet : type %i, from {%i,%i}, to {%i, %i}, next_hop %i, tile_id %i, time %llu", \
            (SInt32) buff_pkt->type, \
            buff_pkt->sender.tile_id, buff_pkt->sender.core_type, \
            hopVec[i].final_dest.tile_id, hopVec[i].final_dest.core_type, \
            hopVec[i].next_dest.tile_id, \
            _tile->getId(), hopVec[i].time);

      buff_pkt->time = hopVec[i].time;
      buff_pkt->receiver = hopVec[i].final_dest;
      buff_pkt->specific = hopVec[i].specific;

      _transport->send(hopVec[i].next_dest.tile_id, buffer, packet.bufferSize());
      
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

   assert(_tile);

   // Convert from core cycle count to network cycle count
   packet.time = convertCycleCount(packet.time,
         _tile->getCore()->getPerformanceModel()->getFrequency(),
         getNetworkModelFromPacketType(packet.type)->getFrequency());

   // Note the start time
   packet.start_time = packet.time;

   // Update Send Counters
   (getNetworkModelFromPacketType(packet.type))->updateSendCounters(packet);

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
            , _core_type(MAIN_CORE_TYPE)
      {
      }
      NetRecvIterator(const std::vector<core_id_t> &v)
            : _mode(SENDER_VECTOR)
            , _senders(&v)
            , _i(0)
            , _core_type(MAIN_CORE_TYPE)
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

      inline core_id_t getCoreId()
      {
         switch (_mode)
         {
         case SENDER_VECTOR:
            return (core_id_t)_senders->at(_i);
         case INT:
            return Sim()->getTileManager()->getMainCoreId(_i);
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
      core_type_t _core_type;
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
         // make sure that this core is the proper destination core for this tile
         if (i->receiver.tile_id != _tile->getId() || i->receiver.core_type != _tile->getCurrentCore()->getCoreType())
            if (i->receiver.tile_id != NetPacket::BROADCAST)
               continue;

         // only find packets that match
         for (sender.reset(); !sender.done(); sender.next())
         {
            if (i->sender.tile_id != sender.getCoreId().tile_id || i->sender.core_type != sender.getCoreId().core_type)
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
   assert(0 <= itr->sender.tile_id && itr->sender.tile_id < _numMod);
   assert(0 <= itr->type && itr->type < NUM_PACKET_TYPES);
   assert((itr->receiver.tile_id == _tile->getId()) || (itr->receiver.tile_id == NetPacket::BROADCAST));

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

   LOG_PRINT("Exiting netRecv : type %i, from {%i,%i}", (SInt32)packet.type, packet.sender.tile_id, packet.sender.core_type);

   return packet;
}

// -- Wrappers

SInt32 Network::netSend(core_id_t dest, PacketType type, const void *buf, UInt32 len)
{
   NetPacket packet;
   assert(_tile);
   assert(_tile->getCurrentCore()->getPerformanceModel()); 
   packet.time = _tile->getCurrentCore()->getPerformanceModel()->getCycleCount();
   packet.sender.tile_id = _tile->getCurrentCore()->getCoreId().tile_id;
   packet.sender.core_type = _tile->getCurrentCore()->getCoreId().core_type;
   packet.receiver.tile_id = dest.tile_id;
   packet.receiver.core_type = dest.core_type;
   packet.length = len;
   packet.type = type;
   packet.data = buf;

   return netSend(packet);
}

SInt32 Network::netBroadcast(PacketType type, const void *buf, UInt32 len)
{
   return netSend((core_id_t) {NetPacket::BROADCAST, -1} , type, buf, len);
}

//NetPacket Network::netRecv(SInt32 src, PacketType type)
NetPacket Network::netRecv(core_id_t src, PacketType type)
{
   NetMatch match;
   match.senders.push_back(src);
   match.types.push_back(type);
   return netRecv(match);
}

//NetPacket Network::netRecv(SInt32 src, PacketType type, UInt32 core_type)
//{
   //NetMatch match;
   //match.senders.push_back(src);
   //match.types.push_back(type);
   //match.core_types.push_back((core_type_t) core_type);
   //return netRecv(match);
//}

//NetPacket Network::netRecvFrom(SInt32 src)
NetPacket Network::netRecvFrom(core_id_t src)
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

void Network::resetModels()
{
   for (int i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      _models[i]->reset();
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
      UInt32 metadata_size = 1 + 2 * Config::getSingleton()->getTileIDLength() + 2;
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
   , specific(-1)
   , length(0)
   , data(0)
{
}

NetPacket::NetPacket(UInt64 t, PacketType ty, SInt32 s,
                     SInt32 r, UInt32 l, const void *d)
   : start_time(0)
   , time(t)
   , type(ty)
   , specific(-1)
   , length(l)
   , data(d)
{
   sender = Sim()->getTileManager()->getMainCoreId(s);
   receiver = Sim()->getTileManager()->getMainCoreId(r);
}


NetPacket::NetPacket(UInt64 t, PacketType ty, core_id_t s,
                     core_id_t r, UInt32 l, const void *d)
   : start_time(0)
   , time(t)
   , type(ty)
   , sender(s)
   , receiver(r)
   , specific(-1)
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
