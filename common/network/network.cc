#include <queue>

#include "transport.h"

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

class NetQueue
{
private:
   priority_queue <NetQueueEntry, vector<NetQueueEntry>, earlier> queue;
   PIN_LOCK _lock;

public:
   NetQueue ()
   {
      InitLock (&_lock);
   }
			
   bool empty ()
   {
      return queue.empty();
   }

   void pop () 
   { 
      queue.pop();
   }
				
   void push(const NetQueueEntry& entry)
   { 
      queue.push(entry); 
   }
				
   const NetQueueEntry& top() const 
   { 
      return queue.top();
   }

   size_t size() const
   {
      return queue.size();
   }

   void lock()
   {
      GetLock(&_lock, 1);
   }

   Int32 unlock()
   {
      return ReleaseLock(&_lock);
   }
};

// -- Ctor -- //

Network::Network(Core *core)
   : _core(core)
{
   
}

// -- Dtor -- //

Network::~Network()
{
}

// -- callbacks -- //

void Network::registerCallback(PacketType type, NetworkCallback callback, void *obj)
{
   assert((unsigned int)type <= MAX_PACKET_TYPE);
   _callbacks[type] = callback;
   _callbackObjs[type] = obj;
}

void Network::unregisterCallback(PacketType type)
{
   assert((unsigned int)type <= MAX_PACKET_TYPE);
   _callbacks[type] = NULL;
}

// -- outputSummary -- //

void Network::outputSummary(ostream &out)
{
}

// -- netPullFromTransport -- //

// Polling function that performs background activities, such as
// pulling from the physical transport layer and routing packets to
// the appropriate queues.

void Network::netPullFromTransport()
{
}

// -- netSend -- //

Int32 Network::netSend(NetPacket packet)
{
}

// -- netRecv -- //

NetPacket Network::netRecv(NetMatch match)
{
}

// -- Wrappers -- //

Int32 netSend(Int32 dest, PacketType type, const void *buf, UInt32 len)
{
}

Int32 Network::netBroadcast(PacketType type, const void *buf, UInt32 len)
{
   return netSend(NetPacket::BROADCAST, type, buf, len);
}

NetPacket netRecvFrom(Int32 src)
{
}

NetPacket netRecvType(PacketType type)
{
}

// -- Internal functions -- //

void* Network::netCreateBuf(NetPacket packet, UInt32* buf_size, UInt64 time)
{
}

void Network::netExPacket(void* buffer, NetPacket &packet, UInt64 &time)
{
}
