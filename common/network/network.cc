#include "transport.h"
#include "core.h"

#include "network.h"

#include "log.h"
#define LOG_DEFAULT_RANK   (_transport->ptCommID())
#define LOG_DEFAULT_MODULE NETWORK

// -- Ctor -- //

Network::Network(Core *core)
   : _core(core)
{
   _numMod = g_config->totalCores();
   _tid = _core->getRank();

   _transport = new Transport();
   _transport->ptInit(_core->getRank(), _numMod);

   _callbacks = new NetworkCallback [NUM_PACKET_TYPES];
   _callbackObjs = new void* [NUM_PACKET_TYPES];
   for (SInt32 i = 0; i < NUM_PACKET_TYPES; i++)
      _callbacks[i] = NULL;

   UInt32 modelTypes[NUM_STATIC_NETWORKS];
   g_config->getNetworkModels(modelTypes);

   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      _models[i] = NetworkModel::createModel(this, modelTypes[i]);

   LOG_PRINT("Initialized.");
}

// -- Dtor -- //

Network::~Network()
{
   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      delete _models[i];

   delete [] _callbackObjs;
   delete [] _callbacks;

   delete _transport;

   LOG_PRINT("Destroyed.");
}

// -- callbacks -- //

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

// -- outputSummary -- //

void Network::outputSummary(std::ostream &out) const
{
   out << "Network summary:\n";
   for (UInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
   {
      out << "  Network model " << i << ":\n";
      _models[i]->outputSummary(out);
   }
}

// -- netPullFromTransport -- //

// Polling function that performs background activities, such as
// pulling from the physical transport layer and routing packets to
// the appropriate queues.

void Network::netPullFromTransport()
{
   do
   {
      NetQueueEntry entry;
      {
         void *buffer;
         buffer = _transport->ptRecv();
         netExPacket(buffer, entry.packet, entry.time);
      }
      
      LOG_PRINT("Pull packet : type %i, from %i, time %llu", (SInt32)entry.packet.type, entry.packet.sender, entry.time);
      assert(0 <= entry.packet.sender && entry.packet.sender < _numMod);
      assert(0 <= entry.packet.type && entry.packet.type < NUM_PACKET_TYPES);

      // was this packet sent to us, or should it just be forwarded?
      if (entry.packet.receiver != _transport->ptCommID())
         {
            forwardPacket(entry.packet);

            // if this isn't a broadcast message, then we shouldn't process it further
            if (entry.packet.receiver != NetPacket::BROADCAST)
               continue;
         }

      // asynchronous I/O support
      NetworkCallback callback = _callbacks[entry.packet.type];

      if (callback != NULL)
         {
            LOG_PRINT("Executing callback on packet : type %i, from %i, time %llu.", (SInt32)entry.packet.type, entry.packet.sender, entry.time);
            assert(0 <= entry.packet.sender && entry.packet.sender < _numMod);
            assert(0 <= entry.packet.type && entry.packet.type < NUM_PACKET_TYPES);

            _core->getPerfModel()->updateCycleCount(entry.time);

            callback(_callbackObjs[entry.packet.type], entry.packet);

            delete [] (Byte*)entry.packet.data;
         }

      // synchronous I/O support
      else
         {
            LOG_PRINT("Enqueuing packet : type %i, from %i, time %llu.", (SInt32)entry.packet.type, entry.packet.sender, entry.time);
            _netQueueCond.acquire();
            _netQueue.push_back(entry);
            LOG_ASSERT_WARNING(_netQueue.size() < 500,
                               "WARNING: Net queue size is %u", _netQueue.size());
            _netQueueCond.release();
            _netQueueCond.broadcast();
         }
   }
   while (_transport->ptQuery());
}

// -- forwardPacket -- //

// FIXME: Can forwardPacket be subsumed by netSend?

void Network::forwardPacket(const NetPacket &packet)
{
   NetworkModel *model = _models[g_type_to_static_network_map[packet.type]];

   vector<NetworkModel::Hop> hopVec;
   model->routePacket(packet, hopVec);

   void *forwardBuffer;
   UInt32 forwardBufferSize;
   forwardBuffer = netCreateBuf(packet, &forwardBufferSize, 0xBEEFCAFE);

   UInt64 *timeStamp = (UInt64*)forwardBuffer;
   assert(*timeStamp == 0xBEEFCAFE);

   for (UInt32 i = 0; i < hopVec.size(); i++)
      {
         *timeStamp = hopVec[i].time;
         _transport->ptSend(hopVec[i].dest, (char*)forwardBuffer, forwardBufferSize);
      }

   delete [] (UInt8*)forwardBuffer;
}

// -- netSend -- //

SInt32 Network::netSend(NetPacket packet)
{
   void *buffer;
   UInt32 bufSize;

   assert(packet.type >= 0 && packet.type < NUM_PACKET_TYPES);
   assert(packet.sender == _transport->ptCommID());

   NetworkModel *model = _models[g_type_to_static_network_map[packet.type]];

   vector<NetworkModel::Hop> hopVec;
   model->routePacket(packet, hopVec);

   buffer = netCreateBuf(packet, &bufSize, 0xBEEFCAFE);

   UInt64 *timeStamp = (UInt64*)buffer;
   assert(*timeStamp == 0xBEEFCAFE);

   for (UInt32 i = 0; i < hopVec.size(); i++)
      {
         LOG_PRINT("Send packet : type %i, to %i, time %llu", (SInt32)packet.type, packet.receiver, hopVec[i].time);
         *timeStamp = hopVec[i].time;
         _transport->ptSend(hopVec[i].dest, (char*)buffer, bufSize);
      }

   delete [] (UInt8*)buffer;

   LOG_PRINT("Sent packet");

   return packet.length;
}

// -- netRecv -- //

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
   NetQueue::iterator entryItr;
   Boolean found;

   found = false;

   NetRecvIterator sender = match.senders.empty() 
      ? NetRecvIterator(_numMod) 
      : NetRecvIterator(match.senders);

   NetRecvIterator type = match.types.empty()
      ? NetRecvIterator((UInt32)NUM_PACKET_TYPES)
      : NetRecvIterator(match.types);

   _netQueueCond.acquire();

   while (!found)
   {
      entryItr = _netQueue.end();

      // check every entry in the queue
      for (NetQueue::iterator i = _netQueue.begin();
           i != _netQueue.end();
           i++)
      {
         // only find packets that match
         for (sender.reset(); !sender.done(); sender.next())
         {
            if (i->packet.sender != (SInt32)sender.get())
               continue;

            for (type.reset(); !type.done(); type.next())
            {
               if (i->packet.type != (PacketType)type.get())
                  continue;

               found = true;

               // find the earliest packet
               if (entryItr == _netQueue.end() ||
                   entryItr->time > i->time)
               {
                  entryItr = i;
               }
            }
         }
      }

      // go to sleep until a packet arrives if none have been found
      if (!found)
      {
         _netQueueCond.wait();
      }
   }

   assert(found == true && entryItr != _netQueue.end());
   assert(0 <= entryItr->packet.sender && entryItr->packet.sender < _numMod);
   assert(0 <= entryItr->packet.type && entryItr->packet.type < NUM_PACKET_TYPES);
   assert(entryItr->packet.receiver == _transport->ptCommID());

   // Copy result
   NetQueueEntry entry = *entryItr;
   _netQueue.erase(entryItr);
   _netQueueCond.release();

   _core->getPerfModel()->updateCycleCount(entry.time);

   LOG_PRINT("Exiting netRecv : type %i, from %i", (SInt32)entry.packet.type, entry.packet.sender);

   return entry.packet;
}

// -- Wrappers -- //

SInt32 Network::netSend(SInt32 dest, PacketType type, const void *buf, UInt32 len)
{
   NetPacket packet;
   packet.sender = _transport->ptCommID();
   packet.receiver = dest;
   packet.length = len;
   packet.type = type;
   packet.data = (void*)buf;

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

// -- Internal functions -- //

void* Network::netCreateBuf(const NetPacket &packet, UInt32* buffer_size, UInt64 time)
{
   Byte *buffer;

   *buffer_size = sizeof(packet.type) + sizeof(packet.sender) +
                     sizeof(packet.receiver) + sizeof(packet.length) +
                     packet.length + sizeof(time);

   buffer = new Byte [*buffer_size];

   Byte *dest = buffer;

   // Time MUST be first based on usage in netSend
   memcpy(dest, (Byte*)&time, sizeof(time));
   dest += sizeof(time);

   memcpy(dest, (Byte*)&packet.type, sizeof(packet.type));
   dest += sizeof(packet.type);

   memcpy(dest, (Byte*)&packet.sender, sizeof(packet.sender));
   dest += sizeof(packet.sender);

   memcpy(dest, (Byte*)&packet.receiver, sizeof(packet.receiver));
   dest += sizeof(packet.receiver);

   memcpy(dest, (Byte*)&packet.length, sizeof(packet.length));
   dest += sizeof(packet.length);

   memcpy(dest, (Byte*)packet.data, packet.length);
   dest += packet.length;

   return (void*)buffer;
}

void Network::netExPacket(void* buffer, NetPacket &packet, UInt64 &time)
{
   Byte *ptr = (Byte*)buffer;

   memcpy((Byte *) &time, ptr, sizeof(time));
   ptr += sizeof(time);

   memcpy((Byte *) &packet.type, ptr, sizeof(packet.type));
   ptr += sizeof(packet.type);

   memcpy((Byte *) &packet.sender, ptr, sizeof(packet.sender));
   ptr += sizeof(packet.sender);

   memcpy((Byte *) &packet.receiver, ptr, sizeof(packet.receiver));
   ptr += sizeof(packet.receiver);

   memcpy((Byte *) &packet.length, ptr, sizeof(packet.length));
   ptr += sizeof(packet.length);

   packet.data = new Byte[packet.length];

   memcpy((Byte *) packet.data, ptr, packet.length);
   ptr += packet.length;

   delete [] (Byte*)buffer;
}
