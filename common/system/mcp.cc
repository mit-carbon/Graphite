#include "mcp.h"

#include <sched.h>
#include <iostream>
using namespace std;

MCP::MCP(Network & network)
   :
      _finished(false),
      _network(network),
      MCP_SERVER_MAX_BUFF(256*1024),
      scratch(new char[MCP_SERVER_MAX_BUFF]),
      syscall_server(_network, send_buff, recv_buff, MCP_SERVER_MAX_BUFF, scratch),
      sync_server(_network, recv_buff),
      network_model_analytical_server(_network, recv_buff)
{
}

MCP::~MCP()
{
   delete[] scratch;
}

void MCP::run()
{
   send_buff.clear();
   recv_buff.clear();

   NetPacket recv_pkt;

   NetMatch match;
   match.types.push_back(MCP_REQUEST_TYPE);
   match.types.push_back(MCP_SYSTEM_TYPE);
   recv_pkt = _network.netRecv(match);

   recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  
   int msg_type;

   recv_buff >> msg_type;

   switch(msg_type)
   {
      case MCP_MESSAGE_SYS_CALL:
         syscall_server.handleSyscall(recv_pkt.sender);
         break;
      case MCP_MESSAGE_QUIT:
         cerr << "MCP::run : Quit message received.\n";
         _finished = true;
         break;
      case MCP_MESSAGE_MUTEX_INIT:
         sync_server.mutexInit(recv_pkt.sender); 
        break;
      case MCP_MESSAGE_MUTEX_LOCK:
         sync_server.mutexLock(recv_pkt.sender);
         break;
      case MCP_MESSAGE_MUTEX_UNLOCK:
         sync_server.mutexUnlock(recv_pkt.sender);
         break;
      case MCP_MESSAGE_COND_INIT:
         sync_server.condInit(recv_pkt.sender);
         break;
      case MCP_MESSAGE_COND_WAIT:
         sync_server.condWait(recv_pkt.sender);
         break;
      case MCP_MESSAGE_COND_SIGNAL:
         sync_server.condSignal(recv_pkt.sender);
         break;
      case MCP_MESSAGE_COND_BROADCAST:
         sync_server.condBroadcast(recv_pkt.sender);
         break;
      case MCP_MESSAGE_BARRIER_INIT:
         sync_server.barrierInit(recv_pkt.sender);
         break;
      case MCP_MESSAGE_BARRIER_WAIT:
         sync_server.barrierWait(recv_pkt.sender);
         break;
      case MCP_MESSAGE_UTILIZATION_UPDATE:
         network_model_analytical_server.update(recv_pkt.sender);
         break;
      default:
         cerr << "Unhandled MCP message type: " << msg_type << " from: " << recv_pkt.sender << endl;
         assert(false);
   }

   delete [] (Byte*)recv_pkt.data;
}

void MCP::finish()
{
   cerr << "MCP::finish : Send MCP quit message\n";

   int msg_type = MCP_MESSAGE_QUIT;
   _network.netSend(g_config->MCPCommID(), MCP_SYSTEM_TYPE, &msg_type, sizeof(msg_type));

   while (!finished())
   {
      sched_yield();
   }

   cerr << "MCP::finish : End" << endl;
}

void MCP::broadcastPacket(NetPacket pkt)
{
   pkt.sender = g_config->MCPCommID();

   // FIXME: Is totalMods() always the right range for commids?
   for (UInt32 commid = 0; commid < g_config->totalMods(); commid++)
      {
         pkt.receiver = commid;
         _network.netSend(pkt);
      }
}

void MCP::forwardPacket(NetPacket pkt)
{
   pkt.sender = g_config->MCPCommID();

   assert(pkt.receiver != -1);
   
   _network.netSend(pkt);
}
