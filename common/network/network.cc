#include <queue>

#include "transport.h"
#include "core.h"

#include "network.h"

// -- NetQueue -- //
//
// A priority queue for network packets.

struct NetQueueEntry
{
   NetPacket packet;
   UInt64 time;
};
                
class Earlier
{
public:
   bool operator() (const NetQueueEntry& first,
                    const NetQueueEntry& second) const
   {
      return first.time <= second.time;
   }
};

class NetQueue : public priority_queue <NetQueueEntry, vector<NetQueueEntry>, Earlier>
{
};

// -- Ctor -- //

Network::Network(Core *core)
   : _core(core)
{
   _numMod = g_config->totalMods();
   _tid = _core->getRank();

   _transport = new Transport();
   _transport->ptInit(_core->getId(), _numMod);

   _netQueue = new NetQueue* [_numMod];
   for (SInt32 i = 0; i < _numMod; i++)
      _netQueue[i] = new NetQueue [NUM_PACKET_TYPES];
   
   _netQueueLock = Lock::create();

   _callbacks = new NetworkCallback [NUM_PACKET_TYPES];
   _callbackObjs = new void* [NUM_PACKET_TYPES];
   for (SInt32 i = 0; i < NUM_PACKET_TYPES; i++)
      _callbacks[i] = NULL;

   UInt32 modelTypes[NUM_STATIC_NETWORKS];
   g_config->getNetworkModels(modelTypes);

   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      _models[i] = NetworkModel::createModel(this, modelTypes[i]);
}

// -- Dtor -- //

Network::~Network()
{
   for (SInt32 i = 0; i < NUM_STATIC_NETWORKS; i++)
      delete _models[i];

   delete [] _callbackObjs;
   delete [] _callbacks;

   delete _netQueueLock;

   for (SInt32 i = 0; i < _numMod; i++)
      delete [] _netQueue[i];
   delete [] _netQueue;

   delete _transport;
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
   out << "TODO: Implement network summary.\n";
}

// -- netPullFromTransport -- //

// Polling function that performs background activities, such as
// pulling from the physical transport layer and routing packets to
// the appropriate queues.

void Network::netPullFromTransport()
{
   NetQueueEntry entry;
   
   // Pull up packets waiting in the physical transport layer
   do {
      {
         void *buffer;
         buffer = _transport->ptRecv();
         netExPacket(buffer, entry.packet, entry.time);
      }

      assert(0 <= entry.packet.sender && entry.packet.sender < _numMod);
      assert(0 <= entry.packet.type && entry.packet.type < NUM_PACKET_TYPES);

      // was this packet sent to us, or should it just be forwarded?
      if (entry.packet.receiver != _transport->ptCommID())
         {
            NetworkModel *model = _models[g_type_to_static_network_map[entry.packet.type]];

            vector<NetworkModel::Hop> hopVec;
            model->routePacket(entry.packet, hopVec);

            void *forwardBuffer;
            UInt32 forwardBufferSize;
            forwardBuffer = netCreateBuf(entry.packet, &forwardBufferSize, 0xBEEFCAFE);

            UInt64 *timeStamp = (UInt64*)forwardBuffer;
            assert(*timeStamp == 0xBEEFCAFE);

            for (UInt32 i = 0; i < hopVec.size(); i++)
               {
                  *timeStamp = hopVec[i].time;
                  _transport->ptSend(hopVec[i].dest, (char*)forwardBuffer, forwardBufferSize);
               }

            delete [] (UInt8*)forwardBuffer;

            // if this isn't a broadcast message, then we shouldn't process it further
            if (entry.packet.receiver != NetPacket::BROADCAST)
               continue;
         }

      // asynchronous I/O support
      assert((UInt32)entry.packet.type < NUM_PACKET_TYPES);
      NetworkCallback callback = _callbacks[entry.packet.type];

      if (callback != NULL)
         {
            assert(0 <= entry.packet.sender && entry.packet.sender < _numMod);
            assert(0 <= entry.packet.type && entry.packet.type < NUM_PACKET_TYPES);

            _core->getPerfModel()->updateCycleCount(entry.time);

            callback(_callbackObjs[entry.packet.type], entry.packet);
         }

      // synchronous I/O support
      else
         {
            _netQueueLock->acquire();
            _netQueue[entry.packet.sender][entry.packet.type].push(entry);
            _netQueueLock->release();
         }
   }
   while(_transport->ptQuery());
}

// -- netSend -- //

SInt32 Network::netSend(NetPacket packet)
{
   void *buffer;
   UInt32 bufSize;

   assert(packet.type >= 0 && packet.type < NUM_PACKET_TYPES);

   NetworkModel *model = _models[g_type_to_static_network_map[packet.type]];

   vector<NetworkModel::Hop> hopVec;
   model->routePacket(packet, hopVec);

   buffer = netCreateBuf(packet, &bufSize, 0xBEEFCAFE);

   UInt64 *timeStamp = (UInt64*)buffer;
   assert(*timeStamp == 0xBEEFCAFE);

   for (UInt32 i = 0; i < hopVec.size(); i++)
      {
         *timeStamp = hopVec[i].time;
         _transport->ptSend(hopVec[i].dest, (char*)buffer, bufSize);
      }

   delete [] (UInt8*)buffer;

   return packet.length;
}

// -- netRecv -- //

NetPacket Network::netRecv(NetMatch match)
{
   NetQueueEntry entry;
   
   SInt32 senderStart = 0;
   SInt32 senderEnd = _numMod;

   SInt32 typeStart = 0;
   SInt32 typeEnd = NUM_PACKET_TYPES;

   if (match.sender_flag)
      {
         assert(0 <= match.sender && match.sender < _numMod);
         senderStart = match.sender;
         senderEnd = senderStart + 1;
      }

   if (match.type_flag)
      {
         assert(0 <= match.type && match.type < NUM_PACKET_TYPES);
         typeStart = match.type;
         typeEnd = typeStart + 1;
      }

   while (true)
      {
         entry.time = 0;

         _netQueueLock->acquire();

         for (SInt32 i = senderStart; i < senderEnd; i++)
            {
               for (SInt32 j = typeStart; j < typeEnd; j++)
                  {
                     if (!(_netQueue[i][j].empty()))
                        {
                           if ((entry.time == 0) || (entry.time > _netQueue[i][j].top().time))
                              {
                                 entry = _netQueue[i][j].top();
                                 break;
                              }
                        }
                  }
            }

         // No match found
         _netQueueLock->release();
         // TODO: Go to sleep until a packet arrives
      }

   assert(0 <= entry.packet.sender && entry.packet.sender < _numMod);
   assert(0 <= entry.packet.type && entry.packet.type < NUM_PACKET_TYPES);

   _netQueue[entry.packet.sender][entry.packet.type].pop();
   _netQueueLock->release();

   // Atomically update the time is the packet time is newer
   _core->getPerfModel()->updateCycleCount(entry.time);

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
   match.sender_flag = true;
   match.sender = src;
   match.type_flag = true;
   match.type = type;
   return netRecv(match);
}

NetPacket Network::netRecvFrom(SInt32 src)
{
   NetMatch match;
   match.sender_flag = true;
   match.sender = src;
   match.type_flag = false;
   return netRecv(match);
}

NetPacket Network::netRecvType(PacketType type)
{
   NetMatch match;
   match.sender_flag = false;
   match.type_flag = true;
   match.type = type;
   return netRecv(match);
}

// -- Internal functions -- //

void* Network::netCreateBuf(NetPacket packet, UInt32* buffer_size, UInt64 time)
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
