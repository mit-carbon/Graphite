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

#include "chip.h"
#include "transport.h"
#include <string>
#include <sstream>

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
   SHARED_MEM_ACK,
   MAX_PACKET_TYPE = SHARED_MEM_ACK
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

      Chip *the_chip;		
      Core *the_core;
      int net_tid;
      int net_num_mod;
      char* netCreateBuf(NetPacket packet);
      void netExPacket(char* buffer, NetPacket &packet, UINT64 &time);
      inline void netEntryTasks();
      //FIXME:
      //This is only here till Jim plugs in his functions, for debugging
      //purposes. To be deleted after that
//      void processSharedMemReq(NetPacket packet); 
      void processUnexpectedSharedMemUpdate(NetPacket packet);
      NetQueue **net_queue;
      Transport *transport;

   public:

      int netInit(Chip *chip, int tid, int num_threads, Core *the_core_arg);
      int netSend(NetPacket packet);
      NetPacket netRecv(NetMatch match);
      bool netQuery(NetMatch match);
	  //checkMessages is a hack to force core to check its messages cpc (can we use an interrupt to call it?
	  //FIXME
	  void netCheckMessages();
	  //TODO make these debug prints  a class with its own method? cpc 
	  void printNetPacket(NetPacket packet);  
	  void printNetMatch(NetMatch match, int receiver);  
	  string packetTypeToString(PacketType type);  
};

#endif
