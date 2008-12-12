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
      static int MCP_rank;     // MPI rank of the process containing the MCP
      static UInt32 MCP_tag;   // The tag to use when sending to the MCP
      static int pt_num_mod;
      static int* dest_ranks;  // Map from comm_id to MPI rank

      //***** Private helper functions *****//

      // Return the process number for this process.  Process numbers are
      //  integers between 0 and (g_config->numProcs() - 1), inclusive.
      static UInt32 ptProcessNum();

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

      /* ==================================================================
       * The MCP does not use these routines anymore. It uses the netowrk
       * instead
       * ==================================================================
       *
      // *************************************************************** //
      // These routines are used to communicate with the central server
      // process (known as the "MCP").  There is exactly one server for
      // the entire simulation but the user shouldn't have to know
      // anything about where it is or how to get to it.
      // *************************************************************** //

      // ptSendToMCP:
      //  buffer: (input) Pointer to buffer of data to send
      //  num_bytes: (input) Number of bytes to send from the buffer
      void  ptSendToMCP(UInt8* buffer, UInt32 num_bytes);

      // ptRecvFromMCP:
      //   num_bytes: (output) Number of bytes received
      //   Returns: A pointer to a buffer filled with the received data
      //   Note: Delete the buffer when you are done with it
      UInt8* ptRecvFromMCP(UInt32* num_bytes);

      // ***** The two below should only be called from the MCP ***** //
      // ptMCPSend:
      //  dest: (input) Comm_ID of the module you are sending to
      //  buffer: (input) Pointer to buffer of data to send
      //  num_bytes: (input) Number of bytes to send from the buffer
      void  ptMCPSend(UInt32 dest, UInt8* buffer, UInt32 num_bytes);

      // ptMCPRecv:
      //   num_bytes: (output) Number of bytes received
      //   Returns: A pointer to a buffer filled with the received data
      //   Note: Delete the buffer when you are done with it
      UInt8* ptMCPRecv(UInt32* num_bytes);

      =================================================================== */

};

#endif


