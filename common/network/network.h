#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <vector>
#include <list>
#include "packet_type.h"
#include "fixed_types.h"
#include "cond.h"
#include "transport.h"
#include "network_model.h"

// TODO: Do we need to support multicast to some (but not all)
// destinations?

class Core;
class Network;

// -- Network Packets -- //

class NetPacket
{
public:
   UInt64 start_time;
   UInt64 time;
   PacketType type;
   SInt32 sender;
   SInt32 receiver;
   UInt32 length;
   const void *data;

   NetPacket();
   explicit NetPacket(Byte*);
   NetPacket(UInt64 time, PacketType type, SInt32 sender, 
             SInt32 receiver, UInt32 length, const void *data);

   UInt32 bufferSize() const;
   Byte *makeBuffer() const;
   
   static const SInt32 BROADCAST = 0xDEADBABE;
};

typedef std::list<NetPacket> NetQueue;

// -- Network Matches -- //

class NetMatch
{
   public:
      std::vector<SInt32> senders;
      std::vector<PacketType> types;
};

// -- Network -- //

// This is the managing class that interacts with the physical
// transport layer to forward packets from source to destination.

class Network
{
   public:

      // -- Ctor, housekeeping, etc. -- //
      Network(Core *core);
      ~Network();

      Core *getCore() const { return _core; }
      Transport::Node *getTransport() const { return _transport; }

      typedef void (*NetworkCallback)(void*, NetPacket);

      void registerCallback(PacketType type,
                            NetworkCallback callback,
                            void *obj);

      void unregisterCallback(PacketType type);

      void outputSummary(std::ostream &out) const;

      void netPullFromTransport();

      // -- Main interface -- //

      SInt32 netSend(const NetPacket& packet);
      NetPacket netRecv(const NetMatch &match);

      // -- Wrappers -- //

      SInt32 netSend(SInt32 dest, PacketType type, const void *buf, UInt32 len);
      SInt32 netBroadcast(PacketType type, const void *buf, UInt32 len);
      NetPacket netRecv(SInt32 src, PacketType type);
      NetPacket netRecvFrom(SInt32 src);
      NetPacket netRecvType(PacketType type);

      void enableModels();
      void disableModels();

   private:
      NetworkModel * _models[NUM_STATIC_NETWORKS];

      NetworkCallback *_callbacks;
      void **_callbackObjs;

      Core *_core;
      Transport::Node *_transport;

      SInt32 _tid;
      SInt32 _numMod;

      NetQueue _netQueue;
      Lock _netQueueLock;
      ConditionVariable _netQueueCond;

      void forwardPacket(const NetPacket &packet);
};

#endif // NETWORK_H
