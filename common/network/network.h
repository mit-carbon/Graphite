// Harshad Kasture
//

#ifndef NETWORK_H
#define NETWORK_H

#include "memory_manager.h"
#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <queue>

// JME: not entirely sure why this is needed...
class Chip;

#include "config.h"
#include "chip.h"
#include "transport.h"
#include <string>
#include <sstream>

extern Config* g_config;

// Define data types

// enums for type of network packet
enum  PacketType 
{
   MIN_PACKET_TYPE = 0, 
   INVALID = MIN_PACKET_TYPE, 
   USER,
   SHARED_MEM_REQ,
   SHARED_MEM_UPDATE_EXPECTED,
   SHARED_MEM_UPDATE_UNEXPECTED,
   SHARED_MEM_ACK,
   MAX_PACKET_TYPE = SHARED_MEM_ACK
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
   bool sender_flag; //if wildcard or not (should you look at sender or not)
   PacketType type;
   bool type_flag;
} NetMatch;


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

//<<<<<<< HEAD:common/network/network.h
      Chip *the_chip;		
      Core *the_core;
      int net_tid;
      int net_num_mod;
//      char* netCreateBuf(NetPacket packet);
//=======
      
      char* netCreateBuf(NetPacket packet, UInt32* buf_size);
      void netExPacket(char* buffer, NetPacket &packet, UINT64 &time);
      void netEntryTasks();
      //FIXME:
      //This is only here till Jim plugs in his functions, for debugging
      //purposes. To be deleted after that
//      void processSharedMemReq(NetPacket packet); 
      void processUnexpectedSharedMemUpdate(NetPacket packet);
      NetQueue **net_queue;
      Transport *transport;

   protected:
//      Chip *the_chip;		
//      int net_tid;
//      int net_num_mod;   // Total number of cores in the simulation

      virtual UINT64 netProcCost(NetPacket packet);
      virtual UINT64 netLatency(NetPacket packet);

   public:

//<<<<<<< HEAD:common/network/network.h
//      int netInit(Chip *chip, int tid, int num_threads, Core *the_core_arg);
//      int netSend(NetPacket packet);
//      NetPacket netRecv(NetMatch match);
//      bool netQuery(NetMatch match);
	  //checkMessages is a hack to force core to check its messages cpc (can we use an interrupt to call it?
	  //FIXME
	  void netCheckMessages();
	  //TODO make these debug prints  a class with its own method? cpc 
	  void printNetPacket(NetPacket packet);  
	  void printNetMatch(NetMatch match, int receiver);  
	  string packetTypeToString(PacketType type);  
//=======
//      Network(Chip *chip, int tid, int num_threads); original, i think it needs the core arg though
      Network(Chip *chip, int tid, int num_threads, Core *the_core_arg);
      virtual ~Network(){};
      
      int netCommID() { return transport->ptCommID(); }
      bool netQuery(NetMatch match);
		
      virtual int netSend(NetPacket packet);
      virtual NetPacket netRecv(NetMatch match);
//>>>>>>> master:common/network/network.h
};

#endif
