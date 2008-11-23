// Jonathan Eastep, Charles Gruenwald, Jason Miller
//
// FIXME: this is a hack. 
//
// Includes temporary hooks for the syscall server. Otherwise, this is
// the syscall server. The runSyscallServer hook function is to be 
// manually inserted in main thread of the user app. It calls the code
// the implements the real server. Putting the hook in the user code's 
// main thread gives the server a place to run; we avoid having to spawn 
//
// a thread or something in the simulator to house the server code.  
#ifndef SYSCALL_SERVER_H
#define SYSCALL_SERVER_H

#include <iostream>
#include "packetize.h"
#include "transport.h"
#include "fixed_types.h"
#include "network.h"

class SyscallServer {
   //Note: These structures are shared with the MCP
   private:
      Network & _network;
      UnstructuredBuffer & send_buff;
      UnstructuredBuffer & recv_buff;
      const UInt32 SYSCALL_SERVER_MAX_BUFF;
      char * const scratch;

   public:
      SyscallServer(Network &network, 
            UnstructuredBuffer &send_buff_, UnstructuredBuffer &recv_buff_,
            const UInt32 SERVER_MAX_BUFF,
            char *scratch_);

      ~SyscallServer();

      void handleSyscall(int comm_id);

   private:
      void marshallOpenCall(int comm_id);
      void marshallReadCall(int comm_id);
      void marshallWriteCall(int comm_id);
      void marshallCloseCall(int comm_id);
      void marshallAccessCall(int comm_id);
};


#endif
