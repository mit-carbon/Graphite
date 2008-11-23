#include "network.h"
#include "chip.h"
#include "debug.h"
#define NETWORK_DEBUG
using namespace std;

Network::Network(int tid, int num_mod, Core* the_core_arg)
{
   int i;
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;

   the_core = the_core_arg;
   net_tid = tid;
   net_num_mod = g_config->totalMods();

   transport = new Transport;
   transport->ptInit(the_core->getRank(), num_mod);
   net_queue = new NetQueue* [net_num_mod];
   for(i = 0; i < net_num_mod; i++)
   {
      net_queue[i] = new NetQueue [num_pac_type];
   }
   
	
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
	stringstream ss;
//	ss << "NetSend Packet Data Addr: " << hex << (int*) packet.data << ", PAYLOAD ADDR: " << hex << ((int*)(packet.data))[1] 
//		<< " Pretending Its a RequestPayload, payload.addr: " << hex << ((RequestPayload*) packet.data)->request_address;
//	debugPrint(net_tid, "NETWORK", ss.str());
#endif


	char *buffer;
   UInt32 buf_size;
   
   // Perform SMEM entry tasks
#ifdef NETWORK_DEBUG   
	debugPrint(net_tid, "NETWORK", "netSend -calling netEntryTasks");
#endif	
   netEntryTasks();
#ifdef NETWORK_DEBUG
	debugPrint(net_tid, "NETWORK", "netSend -finished netEntryTasks");
#endif   

   UINT64 time = the_core->getProcTime() + netLatency(packet) + netProcCost(packet);
	debugPrint (net_tid, "NETWORK", "Before Creating Buffer to send");
   buffer = netCreateBuf(packet, &buf_size, time);
	debugPrint (net_tid, "NETWORK", "Created Buffer to send");
   transport->ptSend(packet.receiver, buffer, buf_size);
   
   the_core->setProcTime(the_core->getProcTime() + netProcCost(packet));

//	debugPrint(net_tid, "NETWORK", "netSend -finished netSend");
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
   
   // Perform network entry tasks
   netEntryTasks();

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
		case SHARED_MEM_UPDATE_EXPECTED:
			return "SHARED_MEM_UPDATE_EXPECTED  ";
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
   	stringstream ss;
		ss <<  "Receiving packet type: " << match.type << " from " << match.sender;
		debugPrint(net_tid, "NETWORK", ss.str());
		printNetMatch(match, net_tid);
#endif  
   
   	loop = true;

   	while(loop)
   	{
      	entry.time = 0; 
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
	 			assert(0 <= match.sender && match.sender < net_num_mod);
	 			assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
         	if( !(net_queue[match.sender][match.type].empty()) )
         	{
               	entry = net_queue[match.sender][match.type].top();
               	sender = match.sender;
               	type = match.type;
               	loop = false;
         	}
      	}
      	else if(match.sender_flag && (!match.type_flag))
      	{
	 			int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         	for (int i = 0; i < num_pac_type; i++)
         	{
            	assert(0 <= match.sender && match.sender < net_num_mod);
            	if( !(net_queue[match.sender][i].empty()) )
            	{
               		if((entry.time == 0) || (entry.time > net_queue[match.sender][i].top().time))
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
            	assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
            	if( !(net_queue[i][match.type].empty()) )
            	{
               		if((entry.time == 0) || (entry.time > net_queue[i][match.type].top().time))
               		{
                  		entry = net_queue[i][match.type].top();
                  		//sender = (PacketType)i;
                  		sender = i;
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
                  		if((entry.time == 0) || (entry.time > net_queue[i][j].top().time))
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
#ifdef NETWORK_DEBUG
				stringstream ss;
				ss <<  "Network received packetType: " << entry.packet.type  << " from " << entry.packet.sender;
				debugPrint(net_tid, "NETWORK",  ss.str());
				ss.str("");
				ss <<  "Clock: " << the_core->getProcTime() << "  packet time stamp: " << entry.time;
				debugPrint(net_tid, "NETWORK", ss.str());
#endif
         	assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
				assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
         	net_queue[entry.packet.sender][entry.packet.type].push(entry);
      	}
			
   	}

   	assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
   	assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
   	net_queue[entry.packet.sender][entry.packet.type].pop();
   	packet = entry.packet;

		if(the_core->getProcTime() < entry.time)
   	{
      	the_core->setProcTime(entry.time);
   	}

#ifdef NETWORK_DEBUG
   	printNetPacket(packet);
		ss.str("");
		ss << "Net Recv Received Packet Data Addr: " << hex << (int*) packet.data << ", PAYLOAD ADDR: " << hex << ((int*)(packet.data))[1];
		debugPrint(net_tid, "NETWORK", ss.str());
   	debugPrint(net_tid, "NETWORK", "netRecv - leaving");
#endif
   
   	return packet;
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
   bool found;
   
   found = false;
   entry.time = the_core->getProcTime();
   
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
      assert(0 <= match.sender && match.sender < net_num_mod);
      assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
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
	 		assert(0 <= match.sender && match.sender < net_num_mod);
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
         assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
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


char* Network::netCreateBuf(NetPacket packet, UInt32* buffer_size, UINT64 time)
{
   char *buffer;
   char *temp;
   int running_length = 0;
   unsigned int i;

   *buffer_size = sizeof(packet.type) + sizeof(packet.sender) +
                     sizeof(packet.receiver) + sizeof(packet.length) +
                     packet.length + sizeof(time);
   buffer = new char [*buffer_size];
   temp = (char*) &(packet.type);

   for(i = 0; i < sizeof(packet.type); i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }
	
   running_length += sizeof(packet.type);
   temp = (char*) &(packet.sender);

   for(i = 0; i < sizeof(packet.sender); i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }

   running_length += sizeof(packet.sender);
   temp = (char*) &(packet.receiver);

   for(i = 0; i < sizeof(packet.receiver); i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }

   running_length += sizeof(packet.receiver);
   temp = (char*) &(packet.length);

   for(i = 0; i < sizeof(packet.length); i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }

   running_length += sizeof(packet.length);
   temp = packet.data;

   for(i = 0; i < packet.length; i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }

   running_length += packet.length;
   temp = (char*) &time;

   for(i = 0; i < sizeof(time); i++)
     {
       assert(running_length+i < *buffer_size);
       buffer[running_length + i] = temp[i];
     }

   return buffer;
};


void Network::netExPacket(char *buffer, NetPacket &packet, UINT64 &time)
{
   unsigned int i;
   int running_length = 0;
   char *ptr;

   // TODO: This is ugly. Clean it up.
   // What is memcpy() for ?

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
   assert (running_length == (sizeof(packet)-sizeof(packet.data)) );
   // There will be memory leaks here !!
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

void Network::netEntryTasks()
{
	// HK
	// FIXME
	// The clock updation model for interrupts could be made smarter
	
   // These are a set of tasks to be performed every time the SMEM layer is
   // entered
   char *buffer;
   NetQueueEntry entry;
   int sender;
   PacketType type;
   
	// Pull up packets waiting in the physical transport layer
   while(transport->ptQuery()) {
		buffer = transport->ptRecv();
      Network::netExPacket(buffer, entry.packet, entry.time);
      assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
      assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
      net_queue[entry.packet.sender][entry.packet.type].push(entry);
   }
   
	do //while(type!=INVALID)
   {
      sender = -1;
      type = INVALID;
      entry.time = 0;
      // FIXME: entry.time = _core->getProcTime();
     
      for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][SHARED_MEM_REQ].empty())
         {
            if((entry.time == 0) || (entry.time >= net_queue[i][SHARED_MEM_REQ].top().time))
            {
              entry = net_queue[i][SHARED_MEM_REQ].top();
              sender = i;
              type = SHARED_MEM_REQ;
            }
         }
      }

		for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][SHARED_MEM_EVICT].empty())
         {
            if((entry.time == 0) || (entry.time >= net_queue[i][SHARED_MEM_EVICT].top().time))
            {
              entry = net_queue[i][SHARED_MEM_EVICT].top();
              sender = i;
              type = SHARED_MEM_EVICT;
            }
         }
      }


      for(int i = 0; i < net_num_mod; i++)
      {
         if(!net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].empty())
         {
            if((entry.time == 0) || (entry.time >= net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].top().time))
            {
               entry = net_queue[i][SHARED_MEM_UPDATE_UNEXPECTED].top();
               sender = i;
               type = SHARED_MEM_UPDATE_UNEXPECTED;
            }
         }
      }

	  if(type == SHARED_MEM_REQ)
      {
//			debugPrint(net_tid, "SMEM", "netEntryTasks shared_meM-req....");
			assert(0 <= sender && sender < net_num_mod);
			assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
         net_queue[sender][type].pop();
         
         the_core->getMemoryManager()->addMemRequest(entry.packet);
			
			if(the_core->getProcTime() < entry.time)
			{
				the_core->setProcTime(entry.time);
			}
      }
		else if(type == SHARED_MEM_EVICT)
		{
			assert(0 <= sender && sender < net_num_mod);
			assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
         net_queue[sender][type].pop();
         
         the_core->getMemoryManager()->forwardWriteBackToDram(entry.packet);

			if(the_core->getProcTime() < entry.time)
			{
				the_core->setProcTime(entry.time);
			}
		}
      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
      {
		  assert(0 <= sender && sender < net_num_mod);
		  assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
        net_queue[sender][type].pop();
        
		  the_core->getMemoryManager()->processUnexpectedSharedMemUpdate(entry.packet);
		  
		  if(the_core->getProcTime() < entry.time)
		  {
			  the_core->setProcTime(entry.time);
		  }
      }

   } while(type != INVALID);
	
//   debugPrint(net_tid, "NETWORK", "netEntryTasks finished....");
}

UINT64 Network::netProcCost(NetPacket packet)
{
      return 0;
};

UINT64 Network::netLatency(NetPacket packet)
{
      return 30;
};


