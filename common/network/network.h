#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <vector>
#include <list>
#include "packet_type.h"
#include "fixed_types.h"
#include "cond.h"
#include "transport.h"

// TODO: Do we need to support multicast to some (but not all)
// destinations?

class Core;
class Network;

// -- Network Packets -- //

class NetPacket
{
public:
   UInt64 time;
   PacketType type;
   SInt32 sender;
   SInt32 receiver;
   UInt32 length;
   const void *data; // *MUST* be last entry of the class

   NetPacket();
   explicit NetPacket(Byte*);
   NetPacket(UInt64 time, PacketType type, SInt32 sender, 
             SInt32 receiver, UInt32 length, const void *data);

   UInt32 bufferSize() const;
   Byte *makeBuffer() const;
   
   static const SInt32 BROADCAST = 0xDEADBABE;
   static const UInt32 BASE_SIZE = sizeof(const void*) + sizeof(PacketType) + 3 * sizeof(UInt32) + sizeof(UInt64);
};

typedef std::list<NetPacket> NetQueue;

// -- Network Matches -- //

class NetMatch
{
   public:
      std::vector<SInt32> senders;
      std::vector<PacketType> types;
};

// -- Network Models -- //

// To implement a new network model, you must implement this routing
// object. To route, take a packet and compute the next hop(s) and the
// time stamp for when that packet will be forwarded.
//   This lets one implement "magic" networks, analytical models,
// realistic hop-by-hop modeling, as well as broadcast models, such as
// a bus or ATAC.  Each static network has its own model object. This
// lets the user network be modeled accurately, while the MCP is a
// stupid magic network.
//   A packet will be dropped if no hops are filled in the nextHops
// vector.
class NetworkModel
{
   public:
      NetworkModel(Network *network) : _network(network) { }
      virtual ~NetworkModel() { }

      struct Hop
      {
         SInt32 dest;
         UInt64 time;
      };

      virtual void routePacket(const NetPacket &pkt,
                               std::vector<Hop> &nextHops) = 0;

      virtual void outputSummary(std::ostream &out) = 0;

      static NetworkModel *createModel(Network *network, UInt32 type);

   protected:
      Network *getNetwork() { return _network; }

   private:
      Network *_network;

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

      typedef void (*NetworkCallback)(void*, NetPacket);

      void registerCallback(PacketType type,
                            NetworkCallback callback,
                            void *obj);

      void unregisterCallback(PacketType type);

      void outputSummary(std::ostream &out) const;

      void netPullFromTransport();

      // -- Main interface -- //

      SInt32 netSend(NetPacket packet);
      NetPacket netRecv(const NetMatch &match);

      // -- Wrappers -- //

      SInt32 netSend(SInt32 dest, PacketType type, const void *buf, UInt32 len);
      SInt32 netBroadcast(PacketType type, const void *buf, UInt32 len);
      NetPacket netRecv(SInt32 src, PacketType type);
      NetPacket netRecvFrom(SInt32 src);
      NetPacket netRecvType(PacketType type);

   private:
      NetworkModel * _models[NUM_STATIC_NETWORKS];

      NetworkCallback *_callbacks;
      void **_callbackObjs;

      Core *_core;
      Transport::Node *_transport;

      SInt32 _tid;
      SInt32 _numMod;

      NetQueue _netQueue;
      ConditionVariable _netQueueCond;

      void forwardPacket(const NetPacket &packet);
};

#endif // NETWORK_H
