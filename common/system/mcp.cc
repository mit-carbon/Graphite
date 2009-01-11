#include "mcp.h"

MCP::MCP(Network & network)
   :
      _finished(false),
      _network(network),
      MCP_SERVER_MAX_BUFF(256*1024),
      scratch(new char[MCP_SERVER_MAX_BUFF]),
      syscall_server(_network, send_buff, recv_buff, MCP_SERVER_MAX_BUFF, scratch),
      sync_server(_network, recv_buff),
      network_mesh_analytical_server(_network, recv_buff)
{
}

MCP::~MCP()
{
   delete[] scratch;
}

void MCP::run()
{
//   cerr << "Waiting for MCP request..." << endl;

   send_buff.clear();
   recv_buff.clear();

   NetPacket recv_pkt;

   recv_pkt = _network.netRecvType(MCP_REQUEST_TYPE); 

   recv_buff << make_pair(recv_pkt.data, recv_pkt.length);
  
   int msg_type;

   recv_buff >> msg_type;

   switch(msg_type)
   {
      case MCP_MESSAGE_SYS_CALL:
         syscall_server.handleSyscall(recv_pkt.sender);
         break;
      case MCP_MESSAGE_QUIT:
         cerr << "Got the quit message... done waiting for MCP messages..." << endl;
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
         network_mesh_analytical_server.update(recv_pkt.sender);
         break;
      default:
         cerr << "Unhandled MCP message type: " << msg_type << " from: " << recv_pkt.sender << endl;
         assert(false);
   }

//   cerr << "Finished MCP request" << endl;
}

void MCP::finish()
{
//   cerr << "Got finish request..." << endl;
   UnstructuredBuffer quit_buff;
   quit_buff.clear();

   int msg_type = MCP_MESSAGE_QUIT;
   quit_buff << msg_type;

//   cerr << "Sending message to MCP to quit..." << endl;
   _finished = true;

   _network.netSend(g_config->MCPCommID(), MCP_REQUEST_TYPE, quit_buff.getBuffer(), quit_buff.size());

   cerr << "End of MCP::finish();" << endl;
}

void MCP::broadcastPacket(NetPacket pkt)
{
   pkt.sender = g_config->MCPCommID();

   // FIXME: Is totalMods() always the right range for commids?
   for (UINT32 commid = 0; commid < g_config->totalMods(); commid++)
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
