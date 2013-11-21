#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <fstream>
#include <vector>
#include <list>
using std::ostream;
using std::ofstream;
using std::vector;
using std::list;

#include "packet_type.h"
#include "fixed_types.h"
#include "cond.h"
#include "semaphore.h"
#include "transport.h"
#include "time_types.h"
#include "dvfs.h"

class Tile;
class Network;
class NetworkModel;

// -- Network Packets -- //

class NetPacket
{
public:
   Time time;
   PacketType type;
   
   core_id_t sender;
   core_id_t receiver;

   SInt32 node_type;
   
   UInt32 length;
   const void *data;

   Time zero_load_delay;
   Time contention_delay;

   NetPacket();
   explicit NetPacket(Byte*);
   NetPacket(Time time, PacketType type, core_id_t sender, 
             core_id_t receiver, UInt32 length, const void *data);
   NetPacket(Time time, PacketType type, SInt32 sender, 
             SInt32 receiver, UInt32 length, const void *data);

   UInt32 bufferSize() const;
   Byte *makeBuffer() const;

   static const SInt32 BROADCAST = 0xDEADBABE;
};

typedef list<NetPacket> NetQueue;

// -- Network Matches -- //

class NetMatch
{
public:
   NetMatch() {receiver = INVALID_CORE_ID;}
   vector<core_id_t> senders;
   vector<PacketType> types;
   core_id_t receiver;
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

   void outputSummary(ostream &out, const Time& target_completion_time) const;

   void netPullFromTransport();

   // -- Main interface -- //

   SInt32 netSend(NetPacket& packet);
   SInt32 netSend(module_t module, NetPacket& packet);
   NetPacket netRecv(const NetMatch &match);

   // -- Wrappers -- //

   SInt32 netSend(core_id_t dest, PacketType type, const void *buf, UInt32 len);
   SInt32 netBroadcast(PacketType type, const void *buf, UInt32 len);
   NetPacket netRecv(core_id_t src, core_id_t recv, PacketType type);
   NetPacket netRecvFrom(core_id_t src, core_id_t recv);
   NetPacket netRecvType(PacketType type, core_id_t recv);

   void enableModels();
   void disableModels();
   
   // -- Network Injection/Ejection Rate Trace -- //
   static void openUtilizationTraceFiles();
   static void closeUtilizationTraceFiles();
   static void outputUtilizationSummary();

   // -- Network Models -- //
   NetworkModel* getNetworkModel(SInt32 network_id) { return _models[network_id]; }
   NetworkModel* getNetworkModelFromPacketType(PacketType packet_type);

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
   
   // -- Network Injection/Ejection Rate Trace -- //
   static bool* _utilizationTraceEnabled;
   static ofstream* _utilizationTraceFiles;

   // Is shortCut available through shared memory
   bool _sharedMemoryShortcutEnabled;

   SInt32 forwardPacket(const NetPacket& packet);
   
   // -- Network Injection/Ejection Rate Trace -- //
   static void computeTraceEnabledNetworks();
};

#endif // NETWORK_H
