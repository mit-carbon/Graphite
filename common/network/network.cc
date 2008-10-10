#include "network.h"
#include "debug.h"
//#define NETWORK_DEBUG
using namespace std;

//<<<<<<< HEAD:common/network/network.cc
//int Network::netInit(Chip *chip, int tid, int num_mod, Core *the_core_arg)
//=======
Network::Network(Chip *chip, int tid, int num_mod, Core* the_core_arg)
//>>>>>>> master:common/network/network.cc
{
   the_chip = chip;
   the_core = the_core_arg;
   int i;
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
   net_tid = tid;
   net_num_mod = g_config->totalMods();
   transport = new Transport;
   transport->ptInit(tid, num_mod);
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
	ss << "NetSend Packet Data Addr: " << hex << (int*) packet.data << ", PAYLOAD ADDR: " << hex << ((int*)(packet.data))[1] 
		<< " Pretending Its a RequestPayload, payload.addr: " << hex << ((RequestPayload*) packet.data)->request_address;
	debugPrint(net_tid, "NETWORK", ss.str());
#endif

   char *buffer;
   UInt32 buf_size;
   
   // Perform SMEM entry tasks
#ifdef NETWORK_DEBUG   
	debugPrint(net_tid, "NETWORK", "netSend -calling netEntryTasks");
#endif	
   netEntryTasks();
//<<<<<<< HEAD:common/network/network.cc
#ifdef NETWORK_DEBUG
	debugPrint(net_tid, "NETWORK", "netSend -finished netEntryTasks");
#endif   
//   buffer = netCreateBuf(packet);
//   transport->ptSend(packet.receiver, buffer, packet.length);
//=======
   
   buffer = netCreateBuf(packet, &buf_size);
   transport->ptSend(packet.receiver, buffer, buf_size);
   the_chip->setProcTime(net_tid, the_chip->getProcTime(net_tid) + netProcCost(packet));

   // FIXME?: Should we be returning buf_size instead?
//>>>>>>> master:common/network/network.cc
   return packet.length;
}

void Network::printNetPacket(NetPacket packet) {
	cout << endl;
	cout << "  [" << net_tid << "] Network Packet (0x" << hex << ((int *) (packet.data))[SH_MEM_REQ_IDX_ADDR] 
//	cout << "  [" << net_tid << "] Network Packet (0x" << hex << ((Payload*) (packet.data))->address; 
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
	         // if(entry.time >= net_queue[match.sender][match.type].top().time)
            // {
               entry = net_queue[match.sender][match.type].top();
               sender = match.sender;
               type = match.type;
               loop = false;
            // }
         }
      }
      else if(match.sender_flag && (!match.type_flag))
      {
	 int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         for(int i = 0; i < num_pac_type; i++)
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
			printf("\n\n\n\n*******************\n\n\n\n");
			stringstream ss;
			ss <<  "Network received packetType: " << entry.packet.type  << " from " << entry.packet.sender;
			debugPrint(net_tid, "NETWORK",  ss.str());
			ss.str("");
			ss <<  "Clock: " << the_chip->getProcTime(net_tid) << "  packet time stamp: " << entry.time;
			debugPrint(net_tid, "NETWORK", ss.str());
			printf("\n\n\n\n*******************\n\n\n\n");
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

//   the_chip->setProcTime(net_tid, (the_chip->getProcTime(net_tid) > entry.time) ? 
//		                   the_chip->getProcTime(net_tid) : entry.time);
	
	if(the_chip->getProcTime(net_tid) < entry.time)
   {
      the_chip->setProcTime(net_tid, entry.time);
   }

#ifdef NETWORK_DEBUG
   printNetPacket(packet);
	ss.str("");
	ss << "Net Recv Received Packet Data Addr: " << hex << (int*) packet.data << ", PAYLOAD ADDR: " << hex << ((int*)(packet.data))[1];
	debugPrint(net_tid, "NETWORK", ss.str());
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


char* Network::netCreateBuf(NetPacket packet, UInt32* buffer_size)
{
   char *buffer;
   char *temp;
   UINT64 time = the_chip->getProcTime(net_tid) + netLatency(packet) + netProcCost(packet);
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
   assert(running_length == sizeof(packet)-sizeof(packet.data));
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
   
//   debugPrint(net_tid, "SMEM", "netEntryTasks start....");
   // Pull up packets waiting in the physical transport layer
   while(transport->ptQuery())
   {
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
		//entry.time = the_chip->getProcTime(net_tid);
     
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
//<<<<<<< HEAD:common/network/network.cc
			assert(0 <= sender && sender < net_num_mod);
			assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
         net_queue[sender][type].pop();
         
#ifdef NETWORK_DEBUG
         debugPrint(net_tid, "NETWORK", "core received shared memory request.");
#endif
         the_core->getMemoryManager()->addMemRequest(entry.packet);
			if(the_chip->getProcTime(net_tid) < entry.time)
			{
				the_chip->setProcTime(net_tid, entry.time);
			}
//         the_core->getMemoryManager()->processSharedMemReq(entry.packet);

#ifdef NETWORK_DEBUG
         debugPrint(net_tid, "NETWORK", "core finished processing shared memory request.");
#endif
      
      }
      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
      {
		  assert(0 <= sender && sender < net_num_mod);
		  assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
        net_queue[sender][type].pop();
      debugPrint(net_tid, "NETWORK", "core processing shared memory unexpected update.");
        //TODO possibly rename this to addUnexpectedShareMemUpdate(packet)
        the_core->getMemoryManager()->processUnexpectedSharedMemUpdate(entry.packet);
		  if(the_chip->getProcTime(net_tid) < entry.time)
		  {
			  the_chip->setProcTime(net_tid, entry.time);
		  }
      debugPrint(net_tid, "NETWORK", "core finished processing shared memory unexpected update.");
      }
//=======
//	assert(0 <= sender && sender < net_num_mod);
//	assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
//	net_queue[sender][type].pop();
	// FIXME:
	// processSharedMemReq will be a memeber function of the shared memory object
	// This function invocation should be replaced by something along the lines of
	// shared_mem_obj->processSharedMemReq(entry.packet)
//	processSharedMemReq(entry.packet);
//      }
//      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
//		{
//		  assert(0 <= sender && sender < net_num_mod);
//		  assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
//		  net_queue[sender][type].pop();
			// FIXME:
         // processUnexpectedSharedMemUpdate will be a memeber function of the shared memory object
         // This function invocation should be replaced by something along the lines of
         // shared_mem_obj->processUnexpectedSharedMemUpdate(entry.packet)
//         processUnexpectedSharedMemUpdate(entry.packet);
//      }
//>>>>>>> master:common/network/network.cc

   } while(type != INVALID);
	
}

UINT64 Network::netProcCost(NetPacket packet)
{
      return 0;
};

UINT64 Network::netLatency(NetPacket packet)
{
      return 0;
};

// Only here for debugging
// To be removed as soon as Jim plugs his function in
/*
void Network::processSharedMemReq(NetPacket packet)
{
   // Do nothing
   // Only for debugging
   // Jim will provide the correct methods for this in the shared memory object
};

void Network::processUnexpectedSharedMemUpdate(NetPacket packet)
{
   // Do nothing
   // Only for debugging
   // Jim will provide the correct methods for this in the shared memory object
};
*/
