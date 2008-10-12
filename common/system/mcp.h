#ifndef MCP_H
#define MCP_H

#include "packetize.h"
#include "transport.h"
#include "syscall_server.h"
#include <iostream>

// externed so the names don't get name-mangled
extern "C" {
   
   void initMCP();
   void runMCP();
   void finiMCP();

}

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
      MCP();
      ~MCP();
      
};

#endif
