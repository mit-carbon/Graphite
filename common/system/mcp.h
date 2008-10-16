#ifndef MCP_H
#define MCP_H

#include "packetize.h"
#include "transport.h"
#include "syscall_server.h"
#include "fixed_types.h"
#include <iostream>

// Different types of messages that get passed to the MCP
typedef enum {
   MCP_MESSAGE_SYS_CALL,
   MCP_MESSAGE_QUIT

} MessageTypes;

class MCP
{
   private:
      Transport pt_endpt;
      UnstructuredBuffer send_buff;
      UnstructuredBuffer recv_buff;
      const UInt32 MCP_SERVER_MAX_BUFF;
      char *scratch;

      SyscallServer syscall_server;

   public:
      void run();
      void finish();
      MCP();
      ~MCP();
      
};

#endif
