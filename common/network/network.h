// Harshad Kasture
//

#ifndef NETWORK_H
#define NETWORK_H

#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <queue>

// JME: not entirely sure why this is needed...
class Chip;

#include "config.h"
//#include "chip.h"
#include "transport.h"

extern Config* g_config;

// Define data types

// enums for type of network packet
enum PacketType 
{
   MIN_PACKET_TYPE = 0, 
   INVALID = MIN_PACKET_TYPE, 
   USER,
   SHARED_MEM_REQ,
   SHARED_MEM_UPDATE_EXPECTED,
   SHARED_MEM_UPDATE_UNEXPECTED,
   MAX_PACKET_TYPE = SHARED_MEM_UPDATE_UNEXPECTED
};

enum NetworkModel
{
    NETWORK_BUS,
    NETWORK_ANALYTICAL_MESH,
    NUM_NETWORK_TYPES
};

// network packet
typedef struct NetPacket
{
   PacketType type;
   int sender;
   int receiver;
   unsigned int length;
   char *data;
} NetPacket;


// network query struct
typedef struct NetMatch
{
   int sender;
   bool sender_flag;
   PacketType type;
   bool type_flag;
} NetMatch;

class Core;

class Network{

   private:

      typedef struct NetQueueEntry{
         NetPacket packet;
	      UINT64 time;
      } NetQueueEntry;
      
      class earlier{
         public:
	    bool operator() (const NetQueueEntry& first, \
            const NetQueueEntry& second) const
	    {
	       return first.time <= second.time;
	    }
      };
		
      typedef priority_queue <NetQueueEntry, vector<NetQueueEntry>, earlier>
              NetQueue;

      
      char* netCreateBuf(NetPacket packet, UInt32* buf_size);
      void netExPacket(char* buffer, NetPacket &packet, UINT64 &time);
      void netEntryTasks();
      //FIXME:
      //This is only here till Jim plugs in his functions, for debugging
      //purposes. To be deleted after that
      void processSharedMemReq(NetPacket packet); 
      void processUnexpectedSharedMemUpdate(NetPacket packet);
      NetQueue **net_queue;
      Transport *transport;

   protected:
      Core *_core;
      int net_num_mod;   // Total number of cores in the simulation

      virtual UINT64 netProcCost(NetPacket packet);
      virtual UINT64 netLatency(NetPacket packet);

   public:

      Network(Core *core, int num_threads);
      virtual ~Network(){};
      
      int netCommID() { return transport->ptCommID(); }
      bool netQuery(NetMatch match);
		
      virtual int netSend(NetPacket packet);
      virtual NetPacket netRecv(NetMatch match);

      Transport *getTransport() { return transport; }
};

#endif
