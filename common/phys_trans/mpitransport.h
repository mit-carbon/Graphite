// Jason Miller
//
// Version of the physical transport layer that uses MPI to communicate
// between cores.

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <iostream>
#include <errno.h>
#include <assert.h>
#include "pin.H"
#include "mpi.h"
#include "config.h"

extern Config* g_config;

class Transport {
   private:
      int    pt_tid;
      int    comm_id;
      bool   i_am_the_MCP;     // True if this node belongs to the MCP
      static UInt32 MCP_tag;   // The tag to use when sending to the MCP
      static int pt_num_mod;
      static int* dest_ranks;

   public:	

      // This routine should be called once within in each process.
      static void ptInitQueue(int num_mod);

      // This routine should be called once within each thread.
      int ptInit(int tid, int num_mod);

      // The MCP should use this initialization routine instead of ptInit
      void ptInitMCP();

      // This routine should be called once when everything is done
      static void ptFinish() { MPI_Finalize(); }

      // Return the communications ID for this node
      int ptCommID() { return comm_id; }

      // Send a message to another core.  This call returns immediately.
      int ptSend(int receiver, char *buffer, int length);

      // Receive the next incoming message from any sender.  This call is
      //  blocking and will not return until a message has been received.
      char* ptRecv();

      // Returns TRUE if there is a message waiting to be received.
      bool ptQuery();

      // These routines are used to communicate with the central server
      //  process (known as the "MCP").  There is exactly one server for
      //  the entire simulation but the user shouldn't have to know
      //  anything about where it is or how to get to it.
      void  ptSendToMCP(UInt8* buffer, UInt32 num_bytes);
      UInt8* ptRecvFromMCP(UInt32* num_bytes);
      // These two should only be called from the MCP
      void  ptMCPSend(UInt32 dest, UInt8* buffer, UInt32 num_bytes);
      UInt8* ptMCPRecv(UInt32* num_bytes);

};

#endif


