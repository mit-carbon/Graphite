#include "network.h"
#include "chip.h"

using namespace std;

Network::Network(Core *core, int num_mod)
  :
      net_queue(new NetQueue* [net_num_mod]),
      transport(new Transport),
      user_queue_lock(new PIN_LOCK [net_num_mod]),
      _core(core)
{
   int i;
   int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
   net_num_mod = g_config->totalMods();
   transport->ptInit(core->getId(), num_mod);
   for(i = 0; i < net_num_mod; i++)
   {
      InitLock(&user_queue_lock[i]);
      net_queue[i] = new NetQueue [num_pac_type];
   }
}

int Network::netSend(NetPacket packet)
{
   char *buffer;
   UInt32 buf_size;
   
   UINT64 time = _core->getProcTime() + netLatency(packet) + netProcCost(packet);
   buffer = netCreateBuf(packet, &buf_size, time);
   transport->ptSend(packet.receiver, buffer, buf_size);
   
   _core->setProcTime(_core->getProcTime() + netProcCost(packet));

   // FIXME?: Should we be returning buf_size instead?
   return packet.length;
}

int Network::netSendMagic(NetPacket packet)
{
   char *buffer;
   UInt32 buf_size;
   
   UINT64 time = _core->getProcTime();
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
   int sender;
   PacketType type;
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
      
      if(match.sender_flag && match.type_flag)
      {
          assert(0 <= match.sender && match.sender < net_num_mod);
          assert(0 <= match.type && match.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);

         if(match.type == USER) 
             GetLock(&user_queue_lock[match.sender], 1);

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
         if(match.type == USER) 
             ReleaseLock(&user_queue_lock[match.sender]);

      }
      else if(match.sender_flag && (!match.type_flag))
      {
         int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         for(int i = 0; i < num_pac_type; i++)
         {
             if(i == USER)
                 GetLock(&user_queue_lock[match.sender], 1);
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
            if(i == USER)
                ReleaseLock(&user_queue_lock[match.sender]);
         }
      }
      else if((!match.sender_flag) && match.type_flag)
      {

         for(int i = 0; i < net_num_mod; i++)
         {
             if(match.type == USER) 
                 GetLock(&user_queue_lock[i], 1);

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

            if(match.type == USER) 
                ReleaseLock(&user_queue_lock[i]);
         }

      }
      else
      {
         int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
         for(int i = 0; i < net_num_mod; i++)
         {
            for(int j = 0; j < num_pac_type; j++)
            {
               if(j == USER)
                   GetLock(&user_queue_lock[i], 1);

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

               if(j == USER)
                   ReleaseLock(&user_queue_lock[i]);

            }
         }
      }


      // No valid packets found in our queue, wait for the
      // "network manager" thread to wake us up
      waitForUserPacket();
   }

   assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
   assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);

   if(entry.packet.type == USER)
       GetLock(&user_queue_lock[entry.packet.sender], 1);

   net_queue[entry.packet.sender][entry.packet.type].pop();

   if(entry.packet.type == USER)
       ReleaseLock(&user_queue_lock[entry.packet.sender]);

   packet = entry.packet;

   //Atomically update the time
   _core->lockClock();
   if(_core->getProcTime() < entry.time)
   {
      _core->setProcTime(entry.time);
   }
   _core->unlockClock();

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
   entry.time = _core->getProcTime();
   
   if(match.sender_flag && match.type_flag)
   {
       if(match.type == USER) 
           GetLock(&user_queue_lock[match.sender], 1);

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

      if(match.type == USER) 
          ReleaseLock(&user_queue_lock[match.sender]);
   }
   else if(match.sender_flag && (!match.type_flag))
   {
      int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
      for(int i = 0; i < num_pac_type; i++)
      {
          if(i == USER)
              GetLock(&user_queue_lock[match.sender], 1);
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
         if(i == USER)
             ReleaseLock(&user_queue_lock[match.sender]);
      }
   }
   else if((!match.sender_flag) && match.type_flag)
   {
      for(int i = 0; i < net_num_mod; i++)
      {
         if(match.type == USER) 
             GetLock(&user_queue_lock[i], 1);

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
         if(match.type == USER) 
             ReleaseLock(&user_queue_lock[i]);
      }
   }
   else
   {
      int num_pac_type = MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1;
      for(int i = 0; i < net_num_mod; i++)
      {
         for(int j = 0; j < num_pac_type; j++)
         {
            if(j == USER)
                GetLock(&user_queue_lock[i], 1);

            if(!net_queue[i][j].empty())
            {
               if(entry.time >= net_queue[i][j].top().time)
               {
                  found = true;
                  entry = net_queue[i][j].top();
                  break;
               }
            }

            if(j == USER)
                ReleaseLock(&user_queue_lock[i]);
         }
      }
   }

   return found;
}

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

void Network::netPullFromTransport()
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
      assert(0 <= entry.packet.sender && entry.packet.sender < net_num_mod);
      assert(0 <= entry.packet.type && entry.packet.type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);

      if(entry.packet.type == USER)
         GetLock(&user_queue_lock[entry.packet.sender], 1);

      net_queue[entry.packet.sender][entry.packet.type].push(entry);

      if(entry.packet.type == USER)
         ReleaseLock(&user_queue_lock[entry.packet.sender]);
   }

   do
   {
      sender = -1;
      type = INVALID;
      entry.time = _core->getProcTime();
     
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
	assert(0 <= sender && sender < net_num_mod);
	assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
	net_queue[sender][type].pop();
	// FIXME:
	// processSharedMemReq will be a memeber function of the shared memory object
	// This function invocation should be replaced by something along the lines of
	// shared_mem_obj->processSharedMemReq(entry.packet)
	processSharedMemReq(entry.packet);
      }
      else if(type == SHARED_MEM_UPDATE_UNEXPECTED)
	{
	  assert(0 <= sender && sender < net_num_mod);
	  assert(0 <= type && type < MAX_PACKET_TYPE - MIN_PACKET_TYPE + 1);
	  net_queue[sender][type].pop();
	  // FIXME:
         // processUnexpectedSharedMemUpdate will be a memeber function of the shared memory object
         // This function invocation should be replaced by something along the lines of
         // shared_mem_obj->processUnexpectedSharedMemUpdate(entry.packet)
         processUnexpectedSharedMemUpdate(entry.packet);
      }

   } while(type != INVALID);
}

void Network::waitForUserPacket()
{

}

void Network::notifyWaitingUserThread()
{

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
}

void Network::processUnexpectedSharedMemUpdate(NetPacket packet)
{
   // Do nothing
   // Only for debugging
   // Jim will provide the correct methods for this in the shared memory object
}

void Network::outputSummary(ostream &out)
{

}

