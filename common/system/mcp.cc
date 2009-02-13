#include "mcp.h"
#include "config.h"

#include "log.h"
#include "core.h"
#define LOG_DEFAULT_RANK _network.getCore()->getId()
#define LOG_DEFAULT_MODULE MCP

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

   LOG_PRINT("MCP message type : %i", (SInt32)msg_type);

   switch(msg_type)
   {
      case MCP_MESSAGE_SYS_CALL:
         syscall_server.handleSyscall(recv_pkt.sender);
         break;
      case MCP_MESSAGE_QUIT:
         LOG_PRINT("Quit message received.");
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
         LOG_ASSERT_ERROR(false, "Unhandled MCP message type: %i from %i", msg_type, recv_pkt.sender);
   }

   delete [] (Byte*)recv_pkt.data;
}

void MCP::finish()
{
   LOG_PRINT("Send MCP quit message");

   int msg_type = MCP_MESSAGE_QUIT;
   _network.netSend(g_config->getMCPCoreNum(), MCP_SYSTEM_TYPE, &msg_type, sizeof(msg_type));

   while (!finished())
   {
      sched_yield();
   }

   LOG_PRINT("MCP Finished.");
}

void MCP::broadcastPacket(NetPacket pkt)
{
   pkt.sender = g_config->getMCPCoreNum();

   // FIXME: Is getTotalCores() always the right range for commids?
   for (UInt32 commid = 0; commid < g_config->getTotalCores(); commid++)
      {
         pkt.receiver = commid;
         _network.netSend(pkt);
      }
}

void MCP::forwardPacket(NetPacket pkt)
{
   pkt.sender = g_config->getMCPCoreNum();

   assert(pkt.receiver != -1);
   
   _network.netSend(pkt);
}
