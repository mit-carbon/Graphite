#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <vector>
#include <list>
#include "packet_type.h"
#include "fixed_types.h"
#include "cond.h"
#include "semaphore.h"
#include "transport.h"
#include "network_model.h"

// TODO: Do we need to support multicast to some (but not all)
// destinations?

class Tile;
class Network;

// -- Network Packets -- //

class NetPacket
{
public:
   UInt64 start_time;
   UInt64 time;
   PacketType type;
   
   core_id_t sender;
   core_id_t receiver;

   //SInt32 sender;
   //SInt32 receiver;
   //UInt32 core_type;

   // This field may be used by specific network models in whatever way they please
   UInt32 specific;
   
   UInt32 length;
   const void *data;

   NetPacket();
   explicit NetPacket(Byte*);
   NetPacket(UInt64 time, PacketType type, core_id_t sender, 
             core_id_t receiver, UInt32 length, const void *data);
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
      //std::vector<SInt32> senders;
      std::vector<core_id_t> senders;
      std::vector<PacketType> types;
      //std::vector<core_type_t> core_types;
};

// -- Network -- //

// This is the managing class that interacts with the physical
// transport layer to forward packets from source to destination.

class Network
{
   public:
      // -- Ctor, housekeeping, etc. -- //
      Network(Tile *tile);
      ~Network();

      Tile *getTile() const { return _tile; }
      Transport::Node *getTransport() const { return _transport; }

      typedef void (*NetworkCallback)(void*, NetPacket);

      void registerCallback(PacketType type,
                            NetworkCallback callback,
                            void *obj);

      void unregisterCallback(PacketType type);

      void outputSummary(std::ostream &out) const;

      void netPullFromTransport();

      // -- Main interface -- //

      SInt32 netSend(NetPacket& packet);
      NetPacket netRecv(const NetMatch &match);

      // -- Wrappers -- //

      SInt32 netSend(core_id_t dest, PacketType type, const void *buf, UInt32 len);
      SInt32 netBroadcast(PacketType type, const void *buf, UInt32 len);
      NetPacket netRecv(core_id_t src, PacketType type);
      NetPacket netRecvFrom(core_id_t src);
      NetPacket netRecvType(PacketType type);

      void enableModels();
      void disableModels();
      void resetModels();

      // -- Network Models -- //
      NetworkModel* getNetworkModelFromPacketType(PacketType packet_type);

      // Modeling
      UInt32 getModeledLength(const NetPacket& pkt);

   private:
      NetworkModel * _models[NUM_STATIC_NETWORKS];

      NetworkCallback *_callbacks;
      void **_callbackObjs;

      Tile *_tile;
      Transport::Node *_transport;

      SInt32 _tid;
      SInt32 _numMod;

      NetQueue _netQueue;
      Lock _netQueueLock;
      ConditionVariable _netQueueCond;
      ConditionVariable _netHelperQueueCond;
      Semaphore _netQueueSem;

      SInt32 forwardPacket(const NetPacket& packet);
};

#endif // NETWORK_H
