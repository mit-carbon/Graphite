#ifndef MCP_H
#define MCP_H

#include "message_types.h"
#include "packetize.h"
#include "network.h"
#include "syscall_server.h"
#include "sync_server.h"
#include "fixed_types.h"
#include "network_model_analytical_server.h"
#include "thread.h"

class MCP : public Runnable
{
   public:
      MCP(Network & network);
      ~MCP();

      void run();
      void processPacket();
      void finish();
      Boolean finished() { return m_finished; };

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
