#ifndef NETWORK_H
#define NETWORK_H

#include "packet_type.h"

class Core;
class Transport;
class NetQueue;

// -- Network Packets -- //

class NetPacket
{
 public:
   PacketType type;
   Int32 sender;
   Int32 receiver;
   UInt32 length;
   void *data;

   NetPacket()
      : type(INVALID)
      , sender(-1)
      , receiver(-1),
      , length(0),
      , data(0)
      {}
};

// -- Network Matches -- //

class NetMatch
{
 public:
   int sender;
   bool sender_flag;
   PacketType type;
   bool type_flag;

   NetMatch()
      : sender(-1)
      , sender_flag(false)
      , type(INVALID)
      , type_flag(false)
      {}
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
   NetworkModel(Network*);
   virtual ~NetworkModel();

   struct Hop
   {
      Int32 dest;
      UInt64 time;
   };

   virtual void routePacket(const NetPacket &pkt,
                            vector<Hop> nextHops) = 0;

   virtual void outputSummary(ostream &out) = 0;

protected:
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

   Transport *getTransport() const { return _transport; }
   Core *getCore() const { return _core; }

   typedef void (*NetworkCallback)(void*, NetPacket);

   void registerCallback(PacketType type,
                         NetworkCallback callback,
                         void *obj);

   void unregisterCallback(PacketType type);

   void outputSummary(ostream &out) const;

   void netPullFromTransport();

   // -- Main interface -- //

   Int32 netSend(NetPacket packet);
   NetPacket netRecv(NetMatch match);

   // -- Wrappers -- //

   Int32 netSend(Int32 dest, PacketType type, const void *buf, UInt32 len);
   NetPacket netRecvFrom(Int32 src);
   NetPacket netRecvType(PacketType type);

private:
   NetworkModel * _models[NUM_STATIC_NETWORKS];

   NetworkCallback *_callbacks;
   void **_callbackObjs;

   Core *_core;
   Transport *_transport;
   
   Int32 _tid;
   Int32 _numMod;

   NetQueue **_netQueue;

   void* netCreateBuf(NetPacket packet, UInt32* buf_size, UInt64 time);
   void netExPacket(void* buffer, NetPacket &packet, UInt64 &time);
};

#endif // NETWORK_H
