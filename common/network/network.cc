#include "network.h"
#include "debug.h"

using namespace std;

int Network::netInit(Chip *chip, int tid, int num_mod, Core *the_core_arg)
{
   the_chip = chip;
   the_core = the_core_arg;
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

void Network::netCheckMessages()
{
	netEntryTasks();
}

int Network::netSend(NetPacket packet)
{
#ifdef NETWORK_DEBUG
	debugPrint(net_tid, "NETWORK", "netSend Begin");
   printNetPacket(packet);
#endif

   char *buffer;
   
   // Perform SMEM entry tasks
#ifdef NETWORK_DEBUG   
	debugPrint(net_tid, "NETWORK", "netSend -calling netEntryTasks");
#endif	
   netEntryTasks();
#ifdef NETWORK_DEBUG
	debugPrint(net_tid, "NETWORK", "netSend -finished netEntryTasks");
#endif   
   buffer = netCreateBuf(packet);
   transport->ptSend(packet.receiver, buffer, packet.length);
   return packet.length;
}

void Network::printNetPacket(NetPacket packet) {
	cout << endl;
	cout << "  [" << net_tid << "] Network Packet (0x" << hex << ((int *) (packet.data))[SH_MEM_REQ_IDX_ADDR] 
		<< ") (" << packet.sender << " -> " << packet.receiver 
		<< ") -- Type: " << packetTypeToString(packet.type) << " ++++" << endl << endl;
}

void Network::printNetMatch(NetMatch match, int receiver) {
	cout << endl;
	cout << "  [" << net_tid << "] Network Match " 
		<< match.sender << " -> " << receiver 
		<< " SenderFlag: " << match.sender_flag
		<< " Type: " << packetTypeToString(match.type)
		<< " TypeFlag: " << match.type_flag 
		<< "----" << endl << endl;
	
//	cout << "   sender_flag : " << match.sender_flag << endl;
//	cout << "   type_flag   : " << match.type_flag << endl << endl;
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
		case SHARED_MEM_UPDATE_EXPECTED:
			return "SHARED_MEM_UPDATE_EXPECTED  ";
		case SHARED_MEM_UPDATE_UNEXPECTED:
			return "SHARED_MEM_UPDATE_UNEXPECTED";
		case SHARED_MEM_ACK:
			return "SHARED_MEM_ACK              ";
	}
	return "ERROR in PacketTypeToString";
}

NetPacket Network::netRecv(NetMatch match)
{
   int sender;
   PacketType type;
   char *buffer;
   NetPacket packet;
   NetQueueEntry entry;
   bool loop;

#ifdef NETWORK_DEBUG
   debugPrint(net_tid, "NETWORK", "netRecv starting...");
	printNetMatch(match, net_tid);
#endif  
   
   loop = true;

   while(loop)
   {
   
      entry.time = the_chip->getProcTime(net_tid);
      // Initialized to garbage values
      sender = -1;
      type = INVALID;
      
      // Perform Network entry tasks
	  #ifdef NETWORK_DEBUG
	  debugPrint(net_tid, "NETWORK", "netRecv -calling netEntryTasks");
	  #endif
	  netEntryTasks();
	  #ifdef NETWORK_DEBUG
	  debugPrint(net_tid, "NETWORK", "netRecv -finished netEntryTasks");
	  #endif

      if(match.sender_flag && match.type_flag)
      {
         if( !(net_queue[match.sender][match.type].empty()) )
         {
	         if(entry.time >= net_queue[match.sender][match.type].top().time)
            {
               entry = net_queue[match.sender][match.type].top();
               sender = match.sender;
               type = match.type;
               loop = false;
            }
         }
      }
      else if(match.sender_flag && (!match.type_flag))
      {
         int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         for(int i = 0; i < num_pac_type; i++)
         {
            if( !(net_queue[match.sender][i].empty()) )
            {
               if(entry.time >= net_queue[match.sender][i].top().time)
               {
                  entry = net_queue[match.sender][i].top();
                  sender = match.sender;
                  type = (PacketType)i;
                  loop = false;
               }
            }
         }
      }
      else if((!match.sender_flag) && match.type_flag)
      {
         for(int i = 0; i < net_num_mod; i++)
         {
            if( !(net_queue[i][match.type].empty()) )
            {
               if(entry.time >= net_queue[i][match.type].top().time)
               {
                  entry = net_queue[i][match.type].top();
                  sender = (PacketType)i;
                  type = match.type;
                  loop = false;
               }
            }
         }
      }
      else
      {
         int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         for(int i = 0; i < net_num_mod; i++)
         {
            for(int j = 0; j < num_pac_type; j++)
            {
               if( !(net_queue[i][j].empty()) )
               {
                  if(entry.time >= net_queue[i][j].top().time)
                  {
                     entry = net_queue[i][j].top();
                     sender = i;
                     type = (PacketType)j;
                     loop = false;
                  }
               }
            }
         }
      }
      
      if(loop)
      {
#ifdef NETWORK_DEBUG         
			debugPrint(net_tid, "NETWORK", "netRecv: calling transport->ptRecv");
#endif			
			buffer = transport->ptRecv();
#ifdef NETWORK_DEBUG         
			debugPrint(net_tid, "NETWORK", "netRecv: finished transport->ptRecv");
#endif			
	      
			Network::netExPacket(buffer, entry.packet, entry.time);
         net_queue[entry.packet.sender][entry.packet.type].push(entry);
      }
			
   }

   net_queue[sender][type].pop();
   packet = entry.packet;
   the_chip->setProcTime(net_tid, (the_chip->getProcTime(net_tid) > entry.time) ? 
		                   the_chip->getProcTime(net_tid) : entry.time);
#ifdef NETWORK_DEBUG
   printNetPacket(packet);
   debugPrint(net_tid, "NETWORK", "netRecv - leaving");
#endif
	return packet;
};

bool Network::netQuery(NetMatch match)
{
   NetQueueEntry entry;
   bool found;
   
   found = false;
   entry.time = the_chip->getProcTime(net_tid);
   
   // HK
   // Perform SMEM entry tasks
#ifdef NETWORK_DEBUG
	debugPrint(net_tid, "NETWORK", "netQuery -calling netEntryTasks");
#endif	
   netEntryTasks(); 
#ifdef NETWORK_DEBUG   
   debugPrint(net_tid, "NETWORK", "netQuery -finished netEntryTasks");
#endif   
   
   if(match.sender_flag && match.type_flag)
   {
      if(!net_queue[match.sender][match.type].empty())
      {
         if(entry.time >= net_queue[match.sender][match.type].top().time)
         {
            found = true;
            entry = net_queue[match.sender][match.type].top();
         }
      }
   }
   else if(match.sender_flag && (!match.type_flag))
   {
      int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
      for(int i = 0; i < num_pac_type; i++)
      {
         if(!net_queue[match.sender][i].empty())
         {
            if(entry.time >= net_queue[match.sender][i].top().time)
            {
               found = true;
               entry = net_queue[match.sender][i].top();
               break;
            }
         }
      }
   }
   else if((!match.sender_flag) && match.type_flag)
   {
      for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][match.type].empty())
         {
            if(entry.time >= net_queue[i][match.type].top().time)
            {
               found = true;
               entry = net_queue[i][match.type].top();
               break;
            }
         }
      }
   }
   else
   {
      int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
      for(int i = 0; i < net_num_mod; i++)
      {
         for(int j = 0; j < num_pac_type; j++)
         {
            if(!net_queue[i][j].empty())
            {
               if(entry.time >= net_queue[i][j].top().time)
               {
                  found = true;
                  entry = net_queue[i][j].top();
                  break;
               }
            }
         }
      }
   }

   if(found)
   {
         return true;
   }
   else
   {
      return false;
   }
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

   // HK
   // De-allocate dynamic memory
   delete [] buffer;
}

inline void Network::netEntryTasks()
{
   // These are a set of tasks to be performed every time the SMEM layer is
   // entered
   char *buffer;
   NetQueueEntry entry;
   int sender;
   PacketType type;
   
//   debugPrint(net_tid, "SMEM", "netEntryTasks start....");
   // Pull up packets waiting in the physical transport layer
   while(transport->ptQuery())
   {
      buffer = transport->ptRecv();
      Network::netExPacket(buffer, entry.packet, entry.time);
      net_queue[entry.packet.sender][entry.packet.type].push(entry);

   }
   
   do
   {
      sender = -1;
      type = INVALID;
      entry.time = the_chip->getProcTime(net_tid);
     
      for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][SHARED_MEM_REQ].empty())
         {
            if(entry.time >= net_queue[i][SHARED_MEM_REQ].top().time)
            {
              entry = net_queue[i][SHARED_MEM_REQ].top();
              sender = i;
              type = SHARED_MEM_REQ;
            }
         }
      }

      for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].empty())
         {
            if(entry.time >= net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].top().time)
            {
               entry = net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].top();
               sender = i;
               type = SHARED_MEM_UPDATE_UNEXPECTED;
            }
         }
      }

	  if(type == SHARED_MEM_REQ)
      {
         net_queue[sender][type].pop();
         
#ifdef NETWORK_DEBUG
         debugPrint(net_tid, "NETWORK", "core received shared memory request.");
#endif
         the_core->getMemoryManager()->processSharedMemReq(entry.packet);

#ifdef NETWORK_DEBUG
         debugPrint(net_tid, "NETWORK", "core finished processing shared memory request.");
#endif
      
      }
      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
      {
         net_queue[sender][type].pop();
		 //TODO for cpc, do code review of below MMU function
         the_core->getMemoryManager()->processUnexpectedSharedMemUpdate(entry.packet);
      }

   } while(type != INVALID);
	
}

