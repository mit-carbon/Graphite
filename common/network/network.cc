#include "network.h"

using namespace std;

Network::Network(Chip *chip, int tid, int num_mod)
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
}

int Network::netSend(NetPacket packet)
{
   char *buffer;
   
   // Perform network entry tasks
   netEntryTasks();
   
   buffer = netCreateBuf(packet);
   transport->ptSend(packet.receiver, buffer, packet.length);
   the_chip->setProcTime(net_tid, the_chip->getProcTime(net_tid) + netProcCost(packet));
   return packet.length;
}


NetPacket Network::netRecv(NetMatch match)
{
   int sender;
   PacketType type;
   char *buffer;
   NetPacket packet;
   NetQueueEntry entry;
   bool loop;

   loop = true;

   while(loop)
   {
  
      entry.time = 0; 
      // Initialized to garbage values
      sender = -1;
      type = INVALID;
      
      // Perform network entry tasks
      netEntryTasks();

      if(match.sender_flag && match.type_flag)
      {
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
            if( !(net_queue[i][match.type].empty()) )
            {
               if((entry.time == 0) || (entry.time > net_queue[i][match.type].top().time))
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
         buffer = transport->ptRecv();
	      Network::netExPacket(buffer, entry.packet, entry.time);
         net_queue[entry.packet.sender][entry.packet.type].push(entry);
      }
			
   }

   net_queue[entry.packet.sender][entry.packet.type].pop();
   packet = entry.packet;
   if(the_chip->getProcTime(net_tid) < entry.time)
   {
      the_chip->setProcTime(net_tid, entry.time);
   }

   return packet;
};

bool Network::netQuery(NetMatch match)
{
   NetQueueEntry entry;
   bool found;
   
   found = false;
   entry.time = the_chip->getProcTime(net_tid);
   
   // HK
   // Perform network entry tasks
   netEntryTasks(); 
   
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
   UINT64 time = the_chip->getProcTime(net_tid) + netLatency(packet) + netProcCost(packet);
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

void Network::netEntryTasks()
{
   // These are a set of tasks to be performed every time the network layer is
   // entered
   char *buffer;
   NetQueueEntry entry;
   int sender;
   PacketType type;
   
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
         // FIXME:
         // processSharedMemReq will be a memeber function of the shared memory object
         // This function invocation should be replaced by something along the lines of
         // shared_mem_obj->processSharedMemReq(entry.packet)
         processSharedMemReq(entry.packet);
      }
      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
      {
         net_queue[sender][type].pop();
         // FIXME:
         // processUnexpectedSharedMemUpdate will be a memeber function of the shared memory object
         // This function invocation should be replaced by something along the lines of
         // shared_mem_obj->processUnexpectedSharedMemUpdate(entry.packet)
         processUnexpectedSharedMemUpdate(entry.packet);
      }

   } while(type != INVALID);
}

UINT64 Network::netProcCost(NetPacket packet)
{
      return 10;
};

UINT64 Network::netLatency(NetPacket packet)
{
      return 30;
};


// FIXME:
// Only here for debugging
// To be removed as soon as Jim plugs his function in
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
