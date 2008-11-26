// Harshad Kasture
//

#ifndef NETWORK_H
#define NETWORK_H


#include <iostream>
#include <sched.h>
#include <unistd.h>
#include <queue>
#include <string>
#include <sstream>

// JME: not entirely sure why this is needed...
class Chip;
#include "packet_type.h"
#include "config.h"
#include "transport.h"
#include "memory_manager.h"

extern Config* g_config;
extern Chip* g_chip; //only here for global_lock debugging purposes

// Define data types

enum NetworkModel
{
    NETWORK_BUS,
    NETWORK_ANALYTICAL_MESH,
    NUM_NETWORK_TYPES
};

// network packet
class NetPacket
{
public:
   PacketType type;
   int sender;
   int receiver;
   unsigned int length;
   char *data;

   NetPacket() : type(INVALID), sender(-1), receiver(-1), length(0), data(NULL) {}
};


// network query struct
typedef struct NetMatch
{
   int sender;
   bool sender_flag; //if wildcard or not (should you look at sender or not)
   PacketType type;
   bool type_flag;
} NetMatch;

class Core;

class Network{

   private:

      //TODO here to try and sort out hangs/crashes when instrumenting 
		//all of shared_memory.  gdb was showing a grasph in stl_push_back,
		//allocate_new.
		
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

		class NetQueue {
			private:
				priority_queue <NetQueueEntry, vector<NetQueueEntry>, earlier> queue;
				PIN_LOCK _lock;
				
			public:

				
				NetQueue ()
				{
					InitLock ( &_lock );
				}
			
				bool empty ()
				{
					return queue.empty ();
				}

				void pop () 
				{ 
					queue.pop (); 
				}
				
				void push ( const NetQueueEntry& entry ) 
				{ 
					queue.push ( entry ); 
				}
				
				const NetQueueEntry& top () const 
				{ 
					return queue.top ();
				}

				size_t size() const
				{
					return queue.size ();
				}

				void lock ()
				{
					GetLock ( &_lock, 1 );
				}

				INT32 unlock ()
				{
					return ReleaseLock ( &_lock );
				}
		};

      char* netCreateBuf(NetPacket packet, UInt32* buf_size, UINT64 time);
      void netExPacket(char* buffer, NetPacket &packet, UINT64 &time);

      void waitForUserPacket();
      void notifyWaitingUserThread();

      //FIXME:
      //This is only here till Jim plugs in his functions, for debugging
      //purposes. To be deleted after that
      void processUnexpectedSharedMemUpdate(NetPacket packet);
      NetQueue **net_queue;
      Transport *transport;

   protected:
      Core *the_core;
      int net_tid;
      int net_num_mod;   // Total number of cores in the simulation

      virtual UINT64 netProcCost(NetPacket packet);
		virtual UINT64 netLatency(NetPacket packet);

   public:

      Network(Core* the_core_arg, int num_mods);
      virtual ~Network(){};
	   
		//checkMessages is a hack to force core to check its messages cpc (can we use an interrupt to call it?
	   //FIXME
	   //TODO make these debug prints  a class with its own method? cpc 
	   void printNetPacket(NetPacket packet);  
	   void printNetMatch(NetMatch match, int receiver);  
	   string packetTypeToString(PacketType type);  
      
      
      int netCommID() { return transport->ptCommID(); }
	   void netCheckMessages();
      bool netQuery(NetMatch match);

      virtual int netSendToMCP(const char *buf, unsigned int len, bool is_magic = false);
      virtual NetPacket netRecvFromMCP();

      virtual int netMCPSend(int commid, const char *buf, unsigned int len, bool is_magic = false);
      virtual NetPacket netMCPRecv();
		
      virtual int netSend(NetPacket packet);
      virtual NetPacket netRecv(NetMatch match);

      int netSendMagic(NetPacket packet);
      NetPacket netRecvMagic(NetMatch match);

      Transport *getTransport() { return transport; }

      virtual void outputSummary(ostream &out) {
			cerr << "Screw you I'm just the Super Class." << endl;
		}

      void netPullFromTransport();
};

#endif
