#ifndef MCP_H
#define MCP_H

#include "message_types.h"
#include "packetize.h"
#include "network.h"
#include "syscall_server.h"
#include "sync_server.h"
#include "fixed_types.h"
#include "network_model_analytical_server.h"
#include <iostream>

/*
// Different types of messages that get passed to the MCP
typedef enum {
   MCP_MESSAGE_SYS_CALL,
   MCP_MESSAGE_QUIT,
   MCP_MESSAGE_MUTEX_INIT,
   MCP_MESSAGE_MUTEX_LOCK,
   MCP_MESSAGE_MUTEX_UNLOCK,
   MCP_MESSAGE_COND_INIT,
   MCP_MESSAGE_COND_WAIT,
   MCP_MESSAGE_COND_SIGNAL,
   MCP_MESSAGE_COND_BROADCAST,
   MCP_MESSAGE_BARRIER_INIT,
   MCP_MESSAGE_BARRIER_WAIT,

} MessageTypes;
*/

class MCP
{
   public:
      MCP(Network & network);
      ~MCP();

      void run();
      void finish();
      Boolean finished() { return m_finished; };

      // These functions expose the MCP network for system use
      void broadcastPacket(NetPacket);
      void broadcastPacketToProcesses(NetPacket);
      void forwardPacket(NetPacket);

   private:
      Boolean m_finished;
      Network & m_network;
      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      const UInt32 m_MCP_SERVER_MAX_BUFF;
      char *m_scratch;

      SyscallServer m_syscall_server;
      SyncServer m_sync_server;
      NetworkModelAnalyticalServer m_network_model_analytical_server;

};

#endif
