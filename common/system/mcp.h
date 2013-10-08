#ifndef MCP_H
#define MCP_H

#include "message_types.h"
#include "packetize.h"
#include "network.h"
#include "vm_manager.h"
#include "syscall_server.h"
#include "sync_server.h"
#include "clock_skew_management_object.h"
#include "fixed_types.h"
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

      VMManager* getVMManager() { return &m_vm_manager; }
      ClockSkewManagementServer* getClockSkewManagementServer() { return m_clock_skew_management_server; }

   private:
      Boolean m_finished;
      Network & m_network;
      UnstructuredBuffer m_send_buff;
      UnstructuredBuffer m_recv_buff;
      const UInt32 m_MCP_SERVER_MAX_BUFF;
      char *m_scratch;

      VMManager m_vm_manager;
      SyscallServer m_syscall_server;
      SyncServer m_sync_server;
      ClockSkewManagementServer* m_clock_skew_management_server;
};

#endif
