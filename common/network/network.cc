#include "network.h"

using namespace std;

int Network::netInit(Chip *chip, int tid, int num_mod)
{
   the_chip = chip;
   int i;
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
   net_tid = tid;
   net_num_mod = num_mod;
   transport = new Transport;
   transport->ptInit(tid, num_mod);
   net_queue = new NetQueue* [net_num_mod];
   for(i = 0; i < net_num_mod; i++)
   {
      net_queue[i] = new NetQueue [num_pac_type];
   }
   return 0;
}

int Network::netSend(NetPacket packet)
{
   char *buffer;
   buffer = netCreateBuf(packet);
   transport->ptSend(packet.receiver, buffer, packet.length);
   return packet.length;
}


NetPacket Network::netRecv(NetMatch match)
{
   char *buffer;
   NetPacket packet;
   NetQueueEntry entry;
   bool loop = true;

   while(loop)
   {
      while(transport->ptQuery()){
         buffer = transport->ptRecv();
	 Network::netExPacket(buffer, entry.packet, entry.time);

	 // HK
	 // De-allocate dynamic memory
	 // delete [] buffer;
         net_queue[entry.packet.sender][entry.packet.type].push(entry);
      }
		
      if( !(net_queue[match.sender][match.type].empty()) )
      {
	 loop = false;
      }
      else
      {
         buffer = transport->ptRecv();
	 Network::netExPacket(buffer, entry.packet, entry.time);
         net_queue[entry.packet.sender][entry.packet.type].push(entry);
      }
			
   }

   entry = net_queue[match.sender][match.type].top();
   net_queue[match.sender][match.type].pop();
   packet = entry.packet;
   the_chip->setProcTime(net_tid, (the_chip->getProcTime(net_tid) > entry.time) ? 
		                   the_chip->getProcTime(net_tid) : entry.time);

   return packet;
};

bool Network::netQuery(NetMatch match)
{
   NetQueueEntry entry;
   entry = net_queue[match.sender][match.type].top();
   if(entry.time > the_chip->getProcTime(net_tid))
      return false;
   else
      return true;
};


char* Network::netCreateBuf(NetPacket packet)
{
   char *buffer;
   char *temp;
   UINT64 time = the_chip->getProcTime(net_tid);
   int running_length = 0;
   unsigned int i;

   int buffer_size = sizeof(packet.type) + sizeof(packet.sender) +
                     sizeof(packet.receiver) + sizeof(packet.length) +
                     packet.length + sizeof(time);
   buffer = new char [buffer_size];
   temp = (char*) &(packet.type);

   for(i = 0; i < sizeof(packet.type); i++)
     buffer[running_length + i] = temp[i];
	
   running_length += sizeof(packet.type);
   temp = (char*) &(packet.sender);

   for(i = 0; i < sizeof(packet.sender); i++)
      buffer[running_length + i] = temp[i];

   running_length += sizeof(packet.sender);
   temp = (char*) &(packet.receiver);

   for(i = 0; i < sizeof(packet.receiver); i++)
      buffer[running_length + i] = temp[i];

   running_length += sizeof(packet.receiver);
   temp = (char*) &(packet.length);

   for(i = 0; i < sizeof(packet.length); i++)
      buffer[running_length + i] = temp[i];

   running_length += sizeof(packet.length);
   temp = packet.data;

   for(i = 0; i < packet.length; i++)
      buffer[running_length + i] = temp[i];

   running_length += packet.length;
   temp = (char*) &time;

   for(i = 0; i < sizeof(time); i++)
      buffer[running_length + i] = temp[i];

   return buffer;
};


void Network::netExPacket(char *buffer, NetPacket &packet, UINT64 &time)
{
   unsigned int i;
   int running_length = 0;
   char *ptr;

   ptr = (char*) &packet.type;
   for(i = 0; i < sizeof(packet.type); i++)
      ptr[i] = buffer[running_length + i];

   running_length += sizeof(packet.type);
   ptr = (char*) &packet.sender;

   for(i = 0; i < sizeof(packet.sender); i++)
      ptr[i] = buffer[running_length + i];
	
   running_length += sizeof(packet.sender);
   ptr = (char*) &packet.receiver;

   for(i = 0; i < sizeof(packet.receiver); i++)
      ptr[i] = buffer[running_length + i];

   running_length += sizeof(packet.receiver);
   ptr = (char*) &packet.length;

   for(i = 0; i < sizeof(packet.length); i++)
      ptr[i] = buffer[running_length + i];

   running_length += sizeof(packet.length);
   packet.data = new char[packet.length];
   ptr = packet.data;

   for(i = 0; i < packet.length; i++)
      ptr[i] = buffer[running_length + i];
	
   running_length += packet.length;
   ptr = (char *)&time;

   for(i = 0; i < sizeof(time); i++)
      ptr[i] = buffer[running_length + i];
}

