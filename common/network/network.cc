#include "network.h"
#include "chip.h"
#include "debug.h"
//#define NETWORK_DEBUG
using namespace std;

Network::Network(Core *the_core_arg, int num_mod)
  :
      net_queue(new NetQueue* [num_mod]),
      transport(new Transport),
      the_core(the_core_arg),
      net_num_mod(num_mod)
//Network::Network(int tid, int num_mod, Core* the_core_arg)
{
   int i;
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;

   the_core = the_core_arg;
   net_tid = the_core->getRank(); //FIXME network needs a unique id to provide debugging
   net_num_mod = g_config->totalMods();

   transport->ptInit(the_core->getId(), num_mod);
//   transport->ptInit(the_core->getRank(), num_mod);

   for(i = 0; i < net_num_mod; i++)
   {
      net_queue[i] = new NetQueue [num_pac_type];
   }

   callbacks = new NetworkCallback [num_pac_type];
   callback_objs = new void* [num_pac_type];
}

Network::~Network()
{
   delete [] callback_objs;
   delete [] callbacks;

   for (int i = 0; i < net_num_mod; i++)
      {
         delete [] net_queue[i];
      }
   delete [] net_queue;

   delete transport;
}

void Network::netCheckMessages()
{
   cerr << "DONT CALL ME" << endl;
}

int Network::netSend(NetPacket packet)
{
	char *buffer;
   UInt32 buf_size;
   
   UINT64 time = the_core->getProcTime() + netLatency(packet) + netProcCost(packet);

   buffer = netCreateBuf(packet, &buf_size, time);
   transport->ptSend(packet.receiver, buffer, buf_size);
   
   the_core->setProcTime(the_core->getProcTime() + netProcCost(packet));

   // FIXME?: Should we be returning buf_size instead?
   return packet.length;
}

void Network::printNetPacket(NetPacket packet) {
	stringstream ss;
	ss << endl;
	ss << "printNetPacket: DON'T CALL ME" << endl;
	debugPrint (net_tid, "NETWORK", ss.str());
}

int Network::netSendMagic(NetPacket packet)
{
   char *buffer;
   UInt32 buf_size;
   
   UINT64 time = the_core->getProcTime();

   buffer = netCreateBuf(packet, &buf_size, time);
   transport->ptSend(packet.receiver, buffer, buf_size);
   
   // FIXME?: Should we be returning buf_size instead?
   return packet.length;
}

NetPacket Network::netRecvMagic(NetMatch match)
{
   return netRecv(match);
}

NetPacket Network::netRecv(NetMatch match)
{
   NetPacket packet;
   NetQueueEntry entry;
   bool loop = true;

   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;

   int sender_start = 0;
   int sender_end = net_num_mod;

   int type_start = 0;
   int type_end = num_pac_type;

   if(match.sender_flag)
   {
       assert(0 <= match.sender && match.sender < net_num_mod);
       sender_start = match.sender;
       sender_end = sender_start + 1;
   }

   if(match.type_flag)
   {
       assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
       type_start = match.type;
       type_end = type_start + 1;
   }

   while(loop)
   {
      // Initialized to garbage values
      entry.time = 0; 

      for(int i = sender_start; i < sender_end; i++)
      {
          for(int j = type_start; j < type_end; j++)
          {
				 net_queue [ i ][ j ] . lock ();

				 if( !(net_queue[i][j].empty()) )
				 {
					 if((entry.time == 0) || (entry.time > net_queue[i][j].top().time))
					 {
						 entry = net_queue[i][j].top();
						 loop = false;
					 }
				 }
				 
				 net_queue [ i ][ j ] . unlock ();
          }
      }
      
      // No valid packets found in our queue, wait for the
      // "network manager" thread to wake us up
      if ( loop ) 
		{
			waitForUserPacket();
		};
   }

   assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
   assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);

   // FIXME: Will the reference stay the same (Note this is a priority queue)?? 
	net_queue[entry.packet.sender][entry.packet.type].lock();
   net_queue[entry.packet.sender][entry.packet.type].pop();
   net_queue[entry.packet.sender][entry.packet.type].unlock();

   packet = entry.packet;

   //Atomically update the time
   the_core->lockClock();
   if(the_core->getProcTime() < entry.time)
      the_core->setProcTime(entry.time);
   the_core->unlockClock();

   return packet;
}

void Network::printNetMatch(NetMatch match, int receiver) {
	stringstream ss;
	ss << endl;
	ss << "Network Match " 
		<< match.sender << " -> " << receiver 
		<< " SenderFlag: " << match.sender_flag
		<< " Type: " << packetTypeToString(match.type)
		<< " TypeFlag: " << match.type_flag 
		<< "----";
	debugPrint (net_tid, "NETWORK", ss.str());
	
}

string Network::packetTypeToString(PacketType type) 
{
	switch(type) {
		case INVALID:
			return "INVALID                     ";
		case USER:
			return "USER                        ";
		case SHARED_MEM_REQ:
			return "SHARED_MEM_REQ              ";
		case SHARED_MEM_RESPONSE:
			return "SHARED_MEM_RESPONSE			";
		case SHARED_MEM_UPDATE_UNEXPECTED:
			return "SHARED_MEM_UPDATE_UNEXPECTED";
		case SHARED_MEM_ACK:
			return "SHARED_MEM_ACK              ";
		case SHARED_MEM_EVICT:
			return "SHARED_MEM_EVICT            ";
		case MCP_NETWORK_TYPE:
			return "MCP_NETWORK_TYPE				";
		default:
			return "ERROR in PacketTypeToString";
	}
	return "ERROR in PacketTypeToString";
}

int Network::netSendToMCP(const char *buf, unsigned int len, bool is_magic)
{
   NetPacket packet;
   packet.sender = netCommID();
   packet.receiver = g_config->totalMods() - 1;
   packet.length = len;
   packet.type = MCP_NETWORK_TYPE;


   //FIXME: This is bad casting away constness
   packet.data = (char *)buf;

   if(is_magic)
      return netSendMagic(packet);
   else
      return netSend(packet);
}

NetPacket Network::netRecvFromMCP()
{
   NetMatch match;
   match.sender = g_config->totalMods() - 1;
   match.sender_flag = true;
   match.type = MCP_NETWORK_TYPE;
   match.type_flag = true;
   return netRecv(match);
}

int Network::netMCPSend(int commid, const char *buf, unsigned int len, bool is_magic)
{
   NetPacket packet;
   packet.sender = g_config->totalMods() - 1;
   packet.receiver = commid;
   packet.length = len;
   packet.type = MCP_NETWORK_TYPE;

   //FIXME: This is bad casting away constness
   packet.data = (char *)buf;
   if(is_magic)
      return netSendMagic(packet);
   else
      return netSend(packet);
}

NetPacket Network::netMCPRecv()
{
   NetMatch match;
   match.sender_flag = false;
   match.type = MCP_NETWORK_TYPE;
   match.type_flag = true;
   return netRecv(match);
}


bool Network::netQuery(NetMatch match)
{
   NetQueueEntry entry;
   bool found = false;
   
	entry.time = the_core->getProcTime();
 
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;

   int sender_start = 0;
   int sender_end = net_num_mod;

   int type_start = 0;
   int type_end = num_pac_type;

   if(match.sender_flag)
   {
       assert(0 <= match.sender && match.sender < net_num_mod);
       sender_start = match.sender;
       sender_end = sender_start + 1;
   }

   if(match.type_flag)
   {
       assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
       type_start = match.type;
       type_end = type_start + 1;
   }

   for(int i = sender_start; i < sender_end; i++)
   {
       for(int j = type_start; j < type_end; j++)
       {
			 //net_queue[i][j].lock();

           if(!net_queue[i][j].empty() && entry.time >= net_queue[i][j].top().time)
           {
               found = true;
               break;
           }

			 //net_queue[i][j].unlock();
       }
   }
   
   return found;
}

char* Network::netCreateBuf(NetPacket packet, UInt32* buffer_size, UINT64 time)
{
   char *buffer;

   *buffer_size = sizeof(packet.type) + sizeof(packet.sender) +
                     sizeof(packet.receiver) + sizeof(packet.length) +
                     packet.length + sizeof(time);

   buffer = new char [*buffer_size];

   char *dest = buffer;

   memcpy(dest, (char*)&packet.type, sizeof(packet.type));
   dest += sizeof(packet.type);

   memcpy(dest, (char*)&packet.sender, sizeof(packet.sender));
   dest += sizeof(packet.sender);

   memcpy(dest, (char*)&packet.receiver, sizeof(packet.receiver));
   dest += sizeof(packet.receiver);

   memcpy(dest, (char*)&packet.length, sizeof(packet.length));
   dest += sizeof(packet.length);

   memcpy(dest, (char*)packet.data, packet.length);
   dest += packet.length;

   memcpy(dest, (char*)&time, sizeof(time));
   dest += sizeof(time);

   return buffer;
}


void Network::netExPacket(char *buffer, NetPacket &packet, UINT64 &time)
{
   char *ptr = buffer;

   memcpy((char *) &packet.type, ptr, sizeof(packet.type));
   ptr += sizeof(packet.type);

   memcpy((char *) &packet.sender, ptr, sizeof(packet.sender));
   ptr += sizeof(packet.sender);

   memcpy((char *) &packet.receiver, ptr, sizeof(packet.receiver));
   ptr += sizeof(packet.receiver);

   memcpy((char *) &packet.length, ptr, sizeof(packet.length));
   ptr += sizeof(packet.length);

   packet.data = new char[packet.length];

   memcpy((char *) packet.data, ptr, packet.length);
   ptr += packet.length;

   memcpy((char *) &time, ptr, sizeof(time));
   ptr += sizeof(time);

   // HK
   // De-allocate dynamic memory
   delete [] buffer;
}

void Network::netPullFromTransport()
{
	// HK
	// FIXME
	// The clock updation model for interrupts could be made smarter
	
   // These are a set of tasks to be performed every time the SMEM layer is
   // entered
   char *buffer;
   NetQueueEntry entry;
   
	// Pull up packets waiting in the physical transport layer
   while(transport->ptQuery()) {
      buffer = transport->ptRecv();
      Network::netExPacket(buffer, entry.packet, entry.time);
      assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
      assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);

      // asynchronous I/O support
      NetworkCallback callback = callbacks[entry.packet.type];

      if (callback != NULL)
         {
            assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
            assert(MIN_PACKET_TYPE <= entry.packet.type && entry.packet.type <= MAX_PACKET_TYPE);

            the_core->lockClock();
            if(the_core->getProcTime() < entry.time)
               {
                  the_core->setProcTime(entry.time);
               }
            the_core->unlockClock();

            callback(callback_objs[entry.packet.type], entry.packet);
         }

      // synchronous I/O support
      else
         {
            // TODO: Performance Consideration
            // We need to lock only when 'entry.packet.type' is one of the following:
            // 1) USER
            // 2) SHARED_MEM_RESPONSE
            // 3) MCP_NETWORK_TYPE
            net_queue[entry.packet.sender][entry.packet.type].lock();
            net_queue[entry.packet.sender][entry.packet.type].push(entry);
            net_queue[entry.packet.sender][entry.packet.type].unlock();
         }
   }
}

void Network::registerCallback(PacketType type, NetworkCallback callback, void *obj)
{
   callbacks[type] = callback;
   callback_objs[type] = obj;
}

void Network::unregisterCallback(PacketType type)
{
   callbacks[type] = NULL;
}

void Network::waitForUserPacket()
{

}

void Network::notifyWaitingUserThread()
{

}

UINT64 Network::netProcCost(NetPacket packet)
{
      return 0;
};

UINT64 Network::netLatency(NetPacket packet)
{
      return 30;
};


