#ifndef MCP_H
#define MCP_H

#include "packetize.h"
#include "network.h"
#include "syscall_server.h"
#include "sync_server.h"
#include "fixed_types.h"
#include <iostream>

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

class MCP
{
   private:
      bool _finished;
      Network & _network;
      UnstructuredBuffer send_buff;
      UnstructuredBuffer recv_buff;
      const UInt32 MCP_SERVER_MAX_BUFF;
      char *scratch;

      SyscallServer syscall_server;
      SyncServer sync_server;

   public:
      void run();
      void finish();
      bool finished() { return _finished; };
      MCP(Network & network);
      ~MCP();
      
};

#endif
